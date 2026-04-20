#pragma once

#include "ScriptProperty.h"
#include "Ember/Script/Script.h"
#include "Ember/ECS/Component/Components.h"
#include "Ember/Core/Application.h"

#include <sol/sol.hpp>
#include <unordered_map>
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
		static std::unordered_map<std::string, ScriptProperty> GetScriptProperties(const SharedPtr<Script>& scriptAsset);

		template<typename T>
		static void SetScriptPropertyOverride(ScriptComponent& component, const std::string& propertyName, T value)
		{
			// Extract ScriptPropertyType from T
			ScriptPropertyType type;
			if constexpr (std::same_as<T, float>)
				type = ScriptPropertyType::Number;
			else if constexpr (std::same_as<T, bool>)
				type = ScriptPropertyType::Bool;
			else if constexpr (std::same_as<T, std::string>)
				type = ScriptPropertyType::String;
			else
			{
				EB_CORE_ASSERT(false, "Unsupported type passed in for prop override!");
				type = ScriptPropertyType::Unknown;
			}
			
			component.UserPropertyOverrides[propertyName] = { propertyName, value, type };

			//auto& assetManager = Application::Instance().GetAssetManager();
			//auto scriptAsset = assetManager.GetAsset<Script>(component.ScriptHandle);
			//EB_CORE_ASSERT(scriptAsset != nullptr, "Script asset not found for handle: {}", component.ScriptHandle);

			//sol::protected_function_result result = GetState().script_file(scriptAsset->GetFilePath());
			//if (!result.valid())
			//{
			//	sol::error err = result;
			//	EB_CORE_ERROR("Failed to load script for setting properties: {0}", err.what());
			//	return;
			//}

			//sol::table scriptClass = result;
			//scriptClass[propertyName] = value;

			//if (component.Instance.valid())
			//	component.Instance[propertyName] = value;
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