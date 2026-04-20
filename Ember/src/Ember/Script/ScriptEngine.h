#pragma once

#include "ScriptProperty.h"
#include "Ember/Script/Script.h"
#include "Ember/ECS/Component/Components.h"
#include "Ember/Core/Application.h"

#include <sol/sol.hpp>
#include <vector>
#include <array>

namespace Ember {

	class Scene;
	struct ScriptComponent;

	class ScriptEngine
	{
	public:
		static void Init();
		static void Shutdown();

		static void BindAPI(Scene* scene);

		static void OnRuntimeStart(Scene* scene);
		static void OnRuntimeStop();

		static sol::state& GetState();
		static std::vector<ScriptProperty> GetScriptProperties(const SharedPtr<Script>& scriptAsset);

		template<typename T>
		static void SetScriptPropertyOverride(ScriptComponent& component, const std::string& propertyName, T value)
		{
			// Extract ScriptPropertyType from T
			ScriptPropertyType type;
			if constexpr (std::same_as<T, bool>)
				type = ScriptPropertyType::Bool;
			else if constexpr (std::same_as<T, int>)
				type = ScriptPropertyType::Int;
			else if constexpr (std::same_as<T, float>)
				type = ScriptPropertyType::Float;
			else if constexpr (std::same_as<T, std::string>)
				type = ScriptPropertyType::String;
			else
			{
				EB_CORE_ASSERT(false, "Unsupported type passed in for prop override!");
				type = ScriptPropertyType::Unknown;
			}
			
			// Set property override
			component.UserPropertyOverrides[propertyName] = { propertyName, value, type };
		}

		inline static std::array<std::string, 5> DefaultEmberFunctions = {
			"OnCreate",
			"OnUpdate",
			"OnOverlapTriggerEnter",
			"OnOverlapTriggerStay",
			"OnOverlapTriggerExit"
		};
	};

}