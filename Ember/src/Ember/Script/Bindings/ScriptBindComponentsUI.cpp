#include "ebpch.h"
#include "ScriptBindComponents.h"
#include "Ember/Core/Application.h"
#include "Ember/ECS/Component/Components.h"
#include "Ember/ECS/System/UIInputSystem.h"
#include "Ember/Script/Bindings/ScriptComponentRef.h"
#include "Ember/Script/ScriptEngine.h"

namespace Ember {

	void BindUIComponents(sol::state& state)
	{
		// Bound as resolving handles (see ScriptComponentRef.h): safe to cache in Lua.
		state.new_usertype<ComponentRef<CanvasComponent>>("CanvasComponent",
			"ReferenceResolution", RefProp(&CanvasComponent::ReferenceResolution),
			"MatchWidthOrHeight", RefProp(&CanvasComponent::MatchWidthOrHeight),
			"SortOrder", RefProp(&CanvasComponent::SortOrder)
		);

		state.new_usertype<ComponentRef<RectTransformComponent>>("RectTransformComponent",
			"AnchorMin", RefProp(&RectTransformComponent::AnchorMin),
			"AnchorMax", RefProp(&RectTransformComponent::AnchorMax),
			"Pivot", RefProp(&RectTransformComponent::Pivot),
			"SizeDelta", RefProp(&RectTransformComponent::SizeDelta),
			"AnchoredPosition", RefProp(&RectTransformComponent::AnchoredPosition),
			"Rotation", RefProp(&RectTransformComponent::Rotation),
			"RaycastTarget", RefProp(&RectTransformComponent::RaycastTarget)
		);

		state.new_enum<UISelectionState>("UISelectionState", {
			{ "Normal", UISelectionState::Normal },
			{ "Highlighted", UISelectionState::Highlighted },
			{ "Pressed", UISelectionState::Pressed },
			{ "Selected", UISelectionState::Selected },
			{ "Disabled", UISelectionState::Disabled }
		});

		state.new_usertype<ComponentRef<UISelectableComponent>>("UISelectableComponent",
			"Interactable", RefProp(&UISelectableComponent::Interactable),
			"NormalColor", RefProp(&UISelectableComponent::NormalColor),
			"HighlightedColor", RefProp(&UISelectableComponent::HighlightedColor),
			"PressedColor", RefProp(&UISelectableComponent::PressedColor),
			"SelectedColor", RefProp(&UISelectableComponent::SelectedColor),
			"DisabledColor", RefProp(&UISelectableComponent::DisabledColor),
			"FadeDuration", RefProp(&UISelectableComponent::FadeDuration),
			"IsHovered", sol::property([](ComponentRef<UISelectableComponent>& ref) { return ref.Resolve().PointerInside; }),
			"IsPressed", sol::property([](ComponentRef<UISelectableComponent>& ref) { return ref.Resolve().PointerDown; }),
			"Select", sol::as_function([](ComponentRef<UISelectableComponent>& ref)
				{
					auto& selectable = ref.Resolve();
					if (!selectable.Interactable)
						return;

					if (auto uiInputSystem = Application::Instance().GetSystemManager().GetSystem<UIInputSystem>())
						uiInputSystem->SetFocusedEntity(ref.Owner.GetEntityHandle());
				})
		);

		// Handlers live in ScriptEngine keyed by UUID, not on the component: the sol::state is
		// destroyed on runtime stop while the scene outlives it, so a handle stored in a component
		// would destruct against a dead lua_State.
		state.new_usertype<ComponentRef<UIButtonComponent>>("UIButtonComponent",
			"WasClickedThisFrame", sol::property([](ComponentRef<UIButtonComponent>& ref) { return ref.Resolve().WasClickedThisFrame; }),
			"OnClick", sol::as_function([](ComponentRef<UIButtonComponent>& ref, sol::protected_function callback)
				{
					ref.Resolve();	// validates the entity still exists
					ScriptEngine::RegisterUICallback(ref.Owner.GetUUID(), UICallbackKind::Click, std::move(callback));
				}),
			"ClearOnClick", sol::as_function([](ComponentRef<UIButtonComponent>& ref)
				{
					ref.Resolve();
					ScriptEngine::ClearUICallbacks(ref.Owner.GetUUID(), UICallbackKind::Click);
				})
		);

		state.new_usertype<ComponentRef<UIToggleComponent>>("UIToggleComponent",
			"IsOn", sol::property(
				[](ComponentRef<UIToggleComponent>& ref) { return ref.Resolve().IsOn; },
				[](ComponentRef<UIToggleComponent>& ref, bool value)
				{
					auto& toggle = ref.Resolve();
					toggle.IsOn = value;
					toggle.VisualStateApplied = false;	// let UIInputSystem move the checkmark
				}),
			"AllowSwitchOff", RefProp(&UIToggleComponent::AllowSwitchOff),
			"WasChangedThisFrame", sol::property([](ComponentRef<UIToggleComponent>& ref) { return ref.Resolve().WasChangedThisFrame; }),
			"OnValueChanged", sol::as_function([](ComponentRef<UIToggleComponent>& ref, sol::protected_function callback)
				{
					ref.Resolve();
					ScriptEngine::RegisterUICallback(ref.Owner.GetUUID(), UICallbackKind::ValueChanged, std::move(callback));
				}),
			"ClearOnValueChanged", sol::as_function([](ComponentRef<UIToggleComponent>& ref)
				{
					ref.Resolve();
					ScriptEngine::ClearUICallbacks(ref.Owner.GetUUID(), UICallbackKind::ValueChanged);
				})
		);

		state.new_enum<TextAlignment>("TextAlignment", {
			{ "Start", TextAlignment::Start },
			{ "Center", TextAlignment::Center },
			{ "End", TextAlignment::End }
			});
	}

}
