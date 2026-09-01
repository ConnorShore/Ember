#pragma once

#include "System.h"
#include "Ember/ECS/Types.h"
#include "Ember/ECS/Registry.h"
#include "Ember/Math/Math.h"
#include "Ember/Input/InputCode.h"

namespace Ember {

	class Scene;

	// The centralized UI input router, equivalent to Unity's EventSystem + GraphicRaycaster.
	// It owns the raycast, hover tracking, pointer capture and focus, and pushes state onto
	// selectables rather than each widget polling input for itself.
	class UIInputSystem : public System
	{
	public:
		UIInputSystem() = default;
		virtual ~UIInputSystem() = default;

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate(TimeStep delta, Scene* scene) override;

		// True when the pointer is over a UI element that accepts raycasts. Gameplay polls this
		// to avoid acting on a click that landed on the HUD.
		bool IsPointerOverUI() const { return m_PointerOverUI; }

		EntityID GetHoveredEntity() const { return m_Hovered; }
		EntityID GetFocusedEntity() const { return m_Focused; }
		void SetFocusedEntity(EntityID entity) { m_Focused = entity; }

		// Exposed so the editor can click-select UI entities; there is no ID-buffer path for UI.
		static EntityID RaycastUI(Scene* scene, const Vector2f& uiPosition);
		static bool IsPointInsideRect(const Matrix4f& worldTransform, const Vector2f& point);

	private:
		void ResolveSelectionStates(Scene* scene, TimeStep delta);
		void ApplyTransition(Scene* scene, EntityID entity, TimeStep delta);
		void Activate(Scene* scene, EntityID entity);
		void SyncToggleVisuals(Scene* scene);
		EntityID FindSelectableInDirection(Scene* scene, EntityID from, const Vector2f& direction) const;
		void Navigate(Scene* scene, const Vector2f& direction);
		bool ConsumeKeyEdge(KeyCode key);
		bool ConsumeGamepadButtonEdge(GamepadButton button);
		bool ConsumeGamepadAxisEdge(GamepadAxis axis, bool positive);

		EntityID FindFirstSelectable(Scene* scene) const;

		// A menu is usually closed by hiding it, which leaves the focus pointing at a selectable that
		// is no longer on screen - and submit would still activate it.
		bool CanReceiveFocus(Scene* scene, EntityID entity) const;
		void DropFocusIfUnusable(Scene* scene);

		EntityID m_Hovered = (EntityID)Constants::Entities::InvalidEntityID;
		EntityID m_Pressed = (EntityID)Constants::Entities::InvalidEntityID;
		EntityID m_Focused = (EntityID)Constants::Entities::InvalidEntityID;

		// EntityIDs are recycled per scene, so carrying these across a scene swap would point them
		// at whatever unrelated entity inherited the handle.
		Scene* m_LastScene = nullptr;

		bool m_PointerOverUI = false;
		bool m_PreviousMouseDown = false;

		// Input exposes level state only, so edges are derived here rather than widening its API.
		std::unordered_map<KeyCode, bool> m_PreviousKeyStates;
		std::unordered_map<GamepadButton, bool> m_PreviousGamepadStates;

		// A stick's two directions need separate latches, or flicking from one to the other reads
		// as "still held" and the second direction never fires.
		std::unordered_map<GamepadAxis, bool> m_PreviousGamepadAxesPositive;
		std::unordered_map<GamepadAxis, bool> m_PreviousGamepadAxesNegative;
	};

}
