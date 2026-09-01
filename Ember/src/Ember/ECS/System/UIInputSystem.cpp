#include "ebpch.h"
#include "UIInputSystem.h"

#include "UILayoutSystem.h"
#include "ScriptSystem.h"
#include "Ember/Core/Application.h"
#include "Ember/Input/Input.h"
#include "Ember/Input/InputActionManager.h"
#include "Ember/Scene/Scene.h"
#include "Ember/Script/ScriptEngine.h"

namespace Ember {

	static constexpr EntityID InvalidEntity = (EntityID)Constants::Entities::InvalidEntityID;

	namespace {

		struct NavBinding
		{
			Vector2f Direction;
			KeyCode Key;
			GamepadButton Button;
			GamepadAxis Axis;
			bool AxisPositive;
		};

		// One control that can submit, paired with whether it fired, so the press can be handed back
		// to InputActionManager as consumed.
		struct SubmitEdge
		{
			InputDevice Device;
			InputControlId Control;
			bool Fired;
		};

		const NavBinding NavBindings[] = {
			{ Vector2f( 0.0f,  1.0f), KeyCode::Up,    GamepadButton::DPadUp,    GamepadAxis::LeftY, true  },
			{ Vector2f( 0.0f, -1.0f), KeyCode::Down,  GamepadButton::DPadDown,  GamepadAxis::LeftY, false },
			{ Vector2f(-1.0f,  0.0f), KeyCode::Left,  GamepadButton::DPadLeft,  GamepadAxis::LeftX, false },
			{ Vector2f( 1.0f,  0.0f), KeyCode::Right, GamepadButton::DPadRight, GamepadAxis::LeftX, true  }
		};

		bool ConsumeEdge(bool down, bool& previousDown)
		{
			bool wasDown = previousDown;
			previousDown = down;
			return down && !wasDown;
		}

		template<typename IsDownFn>
		bool AnyActiveGamepad(IsDownFn&& isDown)
		{
			for (size_t pad = 0; pad < Input::MaxGamepads; pad++)
			{
				if (Input::IsGamepadActive(pad) && isDown(pad))
					return true;
			}

			return false;
		}

	}

	bool UIInputSystem::IsPointInsideRect(const Matrix4f& worldTransform, const Vector2f& point)
	{
		// UILayoutSystem bakes Translate(centre) * RotateZ * Scale(size), so the basis vectors
		// carry the rect's size and orientation. Same unpack the editor gizmo uses.
		Vector2f origin(worldTransform[3][0], worldTransform[3][1]);
		Vector2f rightAxis(worldTransform[0][0], worldTransform[0][1]);
		Vector2f upAxis(worldTransform[1][0], worldTransform[1][1]);

		float rightLength = Math::Length(rightAxis);
		float upLength = Math::Length(upAxis);
		if (rightLength <= 0.001f || upLength <= 0.001f)
			return false;

		Vector2f local = point - origin;
		float localX = Math::Dot(local, rightAxis / rightLength);
		float localY = Math::Dot(local, upAxis / upLength);

		return std::abs(localX) <= rightLength * 0.5f && std::abs(localY) <= upLength * 0.5f;
	}

	EntityID UIInputSystem::RaycastUI(Scene* scene, const Vector2f& uiPosition)
	{
		EntityID hit = InvalidEntity;

		auto uiLayoutSystem = Application::Instance().GetSystemManager().GetSystem<UILayoutSystem>();
		if (!uiLayoutSystem)
			return hit;

		auto& registry = scene->GetRegistry();

		// The list is already in draw order, so the last match is whatever is drawn on top.
		for (const UIDrawEntry& entry : uiLayoutSystem->GetSortedScreenSpaceEntities())
		{
			if (!registry.ContainsComponent<RectTransformComponent>(entry.Entity) || !registry.ContainsComponent<TransformComponent>(entry.Entity))
				continue;

			auto& rect = registry.GetComponent<RectTransformComponent>(entry.Entity);
			if (!rect.RaycastTarget && !registry.ContainsComponent<UISelectableComponent>(entry.Entity))
				continue;

			if (IsPointInsideRect(registry.GetComponent<TransformComponent>(entry.Entity).WorldTransform, uiPosition))
				hit = entry.Entity;
		}

		return hit;
	}

	void UIInputSystem::OnAttach()
	{
		EB_CORE_INFO("UIInputSystem is attached!");
	}

	void UIInputSystem::OnDetach()
	{
		EB_CORE_INFO("UIInputSystem is detached!");
	}

	bool UIInputSystem::ConsumeKeyEdge(KeyCode key)
	{
		return ConsumeEdge(Input::IsKeyDown(key), m_PreviousKeyStates[key]);
	}

	bool UIInputSystem::ConsumeGamepadButtonEdge(GamepadButton button)
	{
		bool down = AnyActiveGamepad([button](size_t pad) { return Input::IsGamepadButtonDown(pad, button); });
		return ConsumeEdge(down, m_PreviousGamepadStates[button]);
	}

	bool UIInputSystem::ConsumeGamepadAxisEdge(GamepadAxis axis, bool positive)
	{
		// Shared with gameplay so both agree on how far the stick counts as pushed.
		const float threshold = Input::ActuationThreshold(Input::StickForAxis(axis));

		bool down = AnyActiveGamepad([axis, positive, threshold](size_t pad)
		{
			float value = Input::GetGamepadAxis(pad, axis);
			return positive ? value > threshold : value < -threshold;
		});

		auto& previousAxes = positive ? m_PreviousGamepadAxesPositive : m_PreviousGamepadAxesNegative;
		return ConsumeEdge(down, previousAxes[axis]);
	}

	void UIInputSystem::Navigate(Scene* scene, const Vector2f& direction)
	{
		// With nothing focused there is nothing to navigate *from*, so the first press adopts a
		// selectable instead of doing nothing. Equivalent to Unity's firstSelectedGameObject, but
		// without making every scene author wire one up.
		if (m_Focused == InvalidEntity)
		{
			m_Focused = FindFirstSelectable(scene);
			return;
		}

		EntityID next = FindSelectableInDirection(scene, m_Focused, direction);
		if (next != InvalidEntity)
			m_Focused = next;
	}

	void UIInputSystem::OnUpdate(TimeStep delta, Scene* scene)
	{
		auto& registry = scene->GetRegistry();

		// Entering Play builds a fresh copy of the scene, and handles are recycled, so stale
		// hover/press/focus would latch onto whatever entity inherited the ID.
		if (scene != m_LastScene)
		{
			m_Hovered = InvalidEntity;
			m_Pressed = InvalidEntity;
			m_Focused = InvalidEntity;
			m_PreviousMouseDown = false;
			m_PreviousKeyStates.clear();
			m_PreviousGamepadStates.clear();
			m_PreviousGamepadAxesPositive.clear();
			m_PreviousGamepadAxesNegative.clear();
			m_LastScene = scene;
		}

		m_PointerOverUI = false;
		for (EntityID entity : registry.ActiveQuery<UIButtonComponent>())
			registry.GetComponent<UIButtonComponent>(entity).WasClickedThisFrame = false;
		for (EntityID entity : registry.ActiveQuery<UIToggleComponent>())
			registry.GetComponent<UIToggleComponent>(entity).WasChangedThisFrame = false;

		EntityID hit = InvalidEntity;
		if (Input::IsViewportInputActive() && Input::IsMouseInViewport())
		{
			hit = RaycastUI(scene, Input::GetViewportMousePosition());
			m_PointerOverUI = hit != InvalidEntity;
		}

		// A non-interactable selectable still blocks the raycast but never activates.
		EntityID hitSelectable = InvalidEntity;
		if (hit != InvalidEntity
			&& registry.ContainsComponent<UISelectableComponent>(hit)
			&& registry.GetComponent<UISelectableComponent>(hit).Interactable)
		{
			hitSelectable = hit;
		}

		bool mouseDown = Input::IsMouseButtonDown(MouseButton::Left);
		bool mousePressed = mouseDown && !m_PreviousMouseDown;
		bool mouseReleased = !mouseDown && m_PreviousMouseDown;
		m_PreviousMouseDown = mouseDown;

		if (hitSelectable != m_Hovered)
		{
			if (m_Hovered != InvalidEntity && registry.ContainsComponent<UISelectableComponent>(m_Hovered))
			{
				registry.GetComponent<UISelectableComponent>(m_Hovered).PointerInside = false;
				ScriptSystem::FireUIEvent(m_Hovered, "OnHoverExit", scene);
			}

			m_Hovered = hitSelectable;

			if (m_Hovered != InvalidEntity)
			{
				registry.GetComponent<UISelectableComponent>(m_Hovered).PointerInside = true;
				ScriptSystem::FireUIEvent(m_Hovered, "OnHoverEnter", scene);
			}
		}

		if (mousePressed)
		{
			m_Pressed = hitSelectable;
			if (m_Pressed != InvalidEntity)
				registry.GetComponent<UISelectableComponent>(m_Pressed).PointerDown = true;

			// Clicking empty space clears focus, matching Unity.
			m_Focused = hitSelectable;
		}

		if (mouseReleased)
		{
			if (m_Pressed != InvalidEntity && registry.ContainsComponent<UISelectableComponent>(m_Pressed))
			{
				registry.GetComponent<UISelectableComponent>(m_Pressed).PointerDown = false;

				// Release must land on the captured target; dragging off cancels the click.
				if (m_Pressed == hitSelectable)
					Activate(scene, m_Pressed);
			}

			m_Pressed = InvalidEntity;
		}

		// The cursor can leave the window mid-press without any release event ever arriving.
		if (!mouseDown && m_Pressed != InvalidEntity)
		{
			if (registry.ContainsComponent<UISelectableComponent>(m_Pressed))
				registry.GetComponent<UISelectableComponent>(m_Pressed).PointerDown = false;
			m_Pressed = InvalidEntity;
		}

		// Every Consume call has to run, so each control gets its own slot rather than short-circuiting
		// into one bool - a skipped call leaves that control's latch stale. Which control fired is
		// kept because only the one that actually submitted may be taken away from gameplay.
		const SubmitEdge submitEdges[] = {
			{ InputDevice::Keyboard, KeyCode::Enter,   ConsumeKeyEdge(KeyCode::Enter) },
			{ InputDevice::Keyboard, KeyCode::Space,   ConsumeKeyEdge(KeyCode::Space) },
			{ InputDevice::Gamepad,  GamepadButton::A, ConsumeGamepadButtonEdge(GamepadButton::A) }
		};

		bool submitPressed = std::any_of(std::begin(submitEdges), std::end(submitEdges),
			[](const SubmitEdge& edge) { return edge.Fired; });

		bool cancelPressed = ConsumeKeyEdge(KeyCode::Escape);
		cancelPressed |= ConsumeGamepadButtonEdge(GamepadButton::B);

		// A hidden or destroyed menu must not keep the focus, or submit would activate a button that
		// is not on screen - and would take the press away from gameplay to do it.
		DropFocusIfUnusable(scene);

		for (const NavBinding& binding : NavBindings)
		{
			bool navPressed = ConsumeKeyEdge(binding.Key);
			navPressed |= ConsumeGamepadButtonEdge(binding.Button);
			navPressed |= ConsumeGamepadAxisEdge(binding.Axis, binding.AxisPositive);

			if (navPressed)
				Navigate(scene, binding.Direction);
		}

		if (submitPressed && m_Focused != InvalidEntity)
		{
			Activate(scene, m_Focused);

			// Gameplay polls input actions from ScriptSystem onwards, all of which run after this
			// system, so an unconsumed press both activates the widget and reaches the player - the
			// A that closes a menu would jump on the same frame.
			auto& inputActions = Application::Instance().GetInputActionManager();
			for (const SubmitEdge& edge : submitEdges)
			{
				if (edge.Fired)
					inputActions.ConsumeControl(edge.Device, edge.Control);
			}
		}

		if (cancelPressed)
			m_Focused = InvalidEntity;

		ResolveSelectionStates(scene, delta);
		SyncToggleVisuals(scene);
	}

	void UIInputSystem::ResolveSelectionStates(Scene* scene, TimeStep delta)
	{
		auto& registry = scene->GetRegistry();
		for (EntityID entity : registry.ActiveQuery<UISelectableComponent>())
		{
			auto& selectable = registry.GetComponent<UISelectableComponent>(entity);

			if (!selectable.Interactable)
			{
				selectable.PointerInside = false;
				selectable.PointerDown = false;
				selectable.State = UISelectionState::Disabled;
			}
			else if (selectable.PointerDown)
				selectable.State = UISelectionState::Pressed;
			else if (selectable.PointerInside)
				selectable.State = UISelectionState::Highlighted;
			else if (entity == m_Focused)
				selectable.State = UISelectionState::Selected;
			else
				selectable.State = UISelectionState::Normal;

			ApplyTransition(scene, entity, delta);
		}
	}

	void UIInputSystem::ApplyTransition(Scene* scene, EntityID entity, TimeStep delta)
	{
		auto& registry = scene->GetRegistry();
		auto& selectable = registry.GetComponent<UISelectableComponent>(entity);
		if (selectable.Transition == UITransitionMode::None)
			return;

		Entity graphic = selectable.TargetGraphicEntity != Constants::InvalidUUID
			? scene->GetEntity(selectable.TargetGraphicEntity)
			: Entity(entity, scene);

		if (graphic == Constants::Entities::InvalidEntityID || !graphic.ContainsComponent<SpriteComponent>())
			return;

		auto& sprite = graphic.GetComponent<SpriteComponent>();

		// Captured lazily so the authored colour survives and tinting never overwrites it.
		if (!selectable.BaseCaptured)
		{
			selectable.BaseColor = sprite.Color;
			selectable.BaseTexture = sprite.TextureHandle;
			selectable.CurrentColor = sprite.Color;
			selectable.BaseCaptured = true;
		}

		if (selectable.Transition == UITransitionMode::ColorTint)
		{
			Vector4f stateColor = selectable.NormalColor;
			switch (selectable.State)
			{
			case UISelectionState::Highlighted: stateColor = selectable.HighlightedColor; break;
			case UISelectionState::Pressed:     stateColor = selectable.PressedColor; break;
			case UISelectionState::Selected:    stateColor = selectable.SelectedColor; break;
			case UISelectionState::Disabled:    stateColor = selectable.DisabledColor; break;
			default: break;
			}

			Vector4f target = selectable.BaseColor * stateColor;
			float t = selectable.FadeDuration <= 0.0f ? 1.0f : std::min(1.0f, delta.Seconds() / selectable.FadeDuration);
			selectable.CurrentColor = Math::Lerp(selectable.CurrentColor, target, t);
			sprite.Color = selectable.CurrentColor;
		}
		else
		{
			UUID stateTexture = Constants::InvalidUUID;
			switch (selectable.State)
			{
			case UISelectionState::Highlighted: stateTexture = selectable.HighlightedTexture; break;
			case UISelectionState::Pressed:     stateTexture = selectable.PressedTexture; break;
			case UISelectionState::Selected:    stateTexture = selectable.SelectedTexture; break;
			case UISelectionState::Disabled:    stateTexture = selectable.DisabledTexture; break;
			default: break;
			}

			sprite.TextureHandle = stateTexture != Constants::InvalidUUID ? stateTexture : selectable.BaseTexture;
		}
	}

	void UIInputSystem::Activate(Scene* scene, EntityID entity)
	{
		auto& registry = scene->GetRegistry();

		if (registry.ContainsComponent<UIButtonComponent>(entity))
		{
			registry.GetComponent<UIButtonComponent>(entity).WasClickedThisFrame = true;
			ScriptSystem::FireUIEvent(entity, "OnClick", scene);
			ScriptEngine::InvokeUICallbacks(scene, entity, UICallbackKind::Click);
		}

		if (!registry.ContainsComponent<UIToggleComponent>(entity))
			return;

		auto& toggle = registry.GetComponent<UIToggleComponent>(entity);

		// In a group the active toggle cannot be switched off by clicking it again.
		if (toggle.GroupEntity != Constants::InvalidUUID && toggle.IsOn && !toggle.AllowSwitchOff)
			return;

		toggle.IsOn = !toggle.IsOn;
		toggle.WasChangedThisFrame = true;
		toggle.VisualStateApplied = false;

		if (toggle.GroupEntity != Constants::InvalidUUID && toggle.IsOn)
		{
			UUID groupEntity = toggle.GroupEntity;
			for (EntityID other : registry.ActiveQuery<UIToggleComponent>())
			{
				if (other == entity)
					continue;

				auto& otherToggle = registry.GetComponent<UIToggleComponent>(other);
				if (otherToggle.GroupEntity != groupEntity || !otherToggle.IsOn)
					continue;

				otherToggle.IsOn = false;
				otherToggle.WasChangedThisFrame = true;
				otherToggle.VisualStateApplied = false;
				ScriptSystem::FireUIEvent(other, "OnValueChanged", scene, false);
				ScriptEngine::InvokeUICallbacks(scene, other, UICallbackKind::ValueChanged, false);
			}
		}

		ScriptSystem::FireUIEvent(entity, "OnValueChanged", scene, toggle.IsOn);
		ScriptEngine::InvokeUICallbacks(scene, entity, UICallbackKind::ValueChanged, toggle.IsOn);
	}

	void UIInputSystem::SyncToggleVisuals(Scene* scene)
	{
		auto& registry = scene->GetRegistry();

		// Runs every frame so a scripted write to IsOn moves the checkmark too.
		for (EntityID entity : registry.ActiveQuery<UIToggleComponent>())
		{
			auto& toggle = registry.GetComponent<UIToggleComponent>(entity);
			if (toggle.VisualStateApplied || toggle.CheckmarkEntity == Constants::InvalidUUID)
				continue;

			Entity checkmark = scene->GetEntity(toggle.CheckmarkEntity);
			if (checkmark != Constants::Entities::InvalidEntityID)
				checkmark.SetActive(toggle.IsOn, true);

			toggle.VisualStateApplied = true;
		}
	}

	bool UIInputSystem::CanReceiveFocus(Scene* scene, EntityID entity) const
	{
		if (entity == InvalidEntity)
			return false;

		auto& registry = scene->GetRegistry();
		if (!registry.ContainsComponent<UISelectableComponent>(entity) || registry.ContainsComponent<DisabledComponent>(entity))
			return false;

		return registry.GetComponent<UISelectableComponent>(entity).Interactable;
	}

	void UIInputSystem::DropFocusIfUnusable(Scene* scene)
	{
		if (m_Focused != InvalidEntity && !CanReceiveFocus(scene, m_Focused))
			m_Focused = InvalidEntity;
	}

	EntityID UIInputSystem::FindFirstSelectable(Scene* scene) const
	{
		auto& registry = scene->GetRegistry();

		// Draw order, so "first" means the topmost canvas's first element rather than whichever
		// entity happens to sit earliest in the component array.
		auto uiLayoutSystem = Application::Instance().GetSystemManager().GetSystem<UILayoutSystem>();
		if (!uiLayoutSystem)
			return InvalidEntity;

		for (const UIDrawEntry& entry : uiLayoutSystem->GetSortedScreenSpaceEntities())
		{
			if (!registry.ContainsComponent<UISelectableComponent>(entry.Entity))
				continue;

			auto& selectable = registry.GetComponent<UISelectableComponent>(entry.Entity);
			if (selectable.Interactable && selectable.Navigation != UINavigationMode::None)
				return entry.Entity;
		}

		return InvalidEntity;
	}

	EntityID UIInputSystem::FindSelectableInDirection(Scene* scene, EntityID from, const Vector2f& direction) const
	{
		auto& registry = scene->GetRegistry();
		if (!registry.ContainsComponent<UISelectableComponent>(from) || !registry.ContainsComponent<RectTransformComponent>(from))
			return InvalidEntity;

		auto& fromSelectable = registry.GetComponent<UISelectableComponent>(from);
		if (fromSelectable.Navigation == UINavigationMode::None)
			return InvalidEntity;

		if (fromSelectable.Navigation == UINavigationMode::Explicit)
		{
			UUID link = Constants::InvalidUUID;
			if (direction.y > 0.5f)       link = fromSelectable.NavigateUp;
			else if (direction.y < -0.5f) link = fromSelectable.NavigateDown;
			else if (direction.x < -0.5f) link = fromSelectable.NavigateLeft;
			else if (direction.x > 0.5f)  link = fromSelectable.NavigateRight;

			Entity target = scene->GetEntity(link);
			return target == Constants::Entities::InvalidEntityID ? InvalidEntity : target.GetEntityHandle();
		}

		auto& fromRect = registry.GetComponent<RectTransformComponent>(from);

		// Search from the rect edge rather than its centre, as Unity's GetPointOnRectEdge does.
		Vector2f edgeDirection = direction / std::max(std::abs(direction.x), std::abs(direction.y));
		Vector2f origin = fromRect.ComputedMin + fromRect.ComputedSize * 0.5f
			+ fromRect.ComputedSize * edgeDirection * 0.5f;

		EntityID best = InvalidEntity;
		float bestScore = -std::numeric_limits<float>::max();

		for (EntityID candidate : registry.ActiveQuery<UISelectableComponent>())
		{
			if (candidate == from || !registry.ContainsComponent<RectTransformComponent>(candidate))
				continue;

			auto& candidateSelectable = registry.GetComponent<UISelectableComponent>(candidate);
			if (!candidateSelectable.Interactable || candidateSelectable.Navigation == UINavigationMode::None)
				continue;

			auto& candidateRect = registry.GetComponent<RectTransformComponent>(candidate);
			Vector2f toCandidate = (candidateRect.ComputedMin + candidateRect.ComputedSize * 0.5f) - origin;

			float alignment = Math::Dot(direction, toCandidate);
			if (alignment <= 0.0f)
				continue;

			float squaredLength = Math::Dot(toCandidate, toCandidate);
			if (squaredLength <= 0.0001f)
				continue;

			// Unity's scoring: favours candidates that are both well-aligned and close.
			float score = alignment / squaredLength;
			if (score > bestScore)
			{
				bestScore = score;
				best = candidate;
			}
		}

		return best;
	}

}
