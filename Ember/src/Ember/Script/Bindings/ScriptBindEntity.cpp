#include "ebpch.h"
#include "ScriptBindEntity.h"

#include "Ember/ECS/Component/Components.h"
#include "Ember/Scene/Entity.h"

namespace Ember {

	static sol::object GetComponentFromString(const std::string& componentTypeStr, Entity& entity, sol::state& state)
	{
		auto guardGet = [&](bool has, auto getter) -> sol::object
		{
			if (!has)
			{
				EB_CORE_ERROR("Entity '{}' (ID: {}) does not have component '{}'!", entity.GetName(), entity.GetEntityHandle(), componentTypeStr);
				return sol::lua_nil;
			}
			return getter();
		};

		if (componentTypeStr == "IDComponent")
			return guardGet(entity.ContainsComponent<IDComponent>(), [&]{ return sol::make_object(state, &entity.GetComponent<IDComponent>()); });
		if (componentTypeStr == "TagComponent")
			return guardGet(entity.ContainsComponent<TagComponent>(), [&]{ return sol::make_object(state, &entity.GetComponent<TagComponent>()); });
		if (componentTypeStr == "RelationshipComponent")
			return guardGet(entity.ContainsComponent<RelationshipComponent>(), [&]{ return sol::make_object(state, &entity.GetComponent<RelationshipComponent>()); });
		if (componentTypeStr == "TransformComponent")
			return guardGet(entity.ContainsComponent<TransformComponent>(), [&]{ return sol::make_object(state, &entity.GetComponent<TransformComponent>()); });
		if (componentTypeStr == "RigidBodyComponent")
			return guardGet(entity.ContainsComponent<RigidBodyComponent>(), [&]{ return sol::make_object(state, &entity.GetComponent<RigidBodyComponent>()); });
		if (componentTypeStr == "SpriteComponent")
			return guardGet(entity.ContainsComponent<SpriteComponent>(), [&]{ return sol::make_object(state, &entity.GetComponent<SpriteComponent>()); });
		if (componentTypeStr == "StaticMeshComponent")
			return guardGet(entity.ContainsComponent<StaticMeshComponent>(), [&]{ return sol::make_object(state, &entity.GetComponent<StaticMeshComponent>()); });
		if (componentTypeStr == "SkinnedMeshComponent")
			return guardGet(entity.ContainsComponent<SkinnedMeshComponent>(), [&]{ return sol::make_object(state, &entity.GetComponent<SkinnedMeshComponent>()); });
		if (componentTypeStr == "MaterialComponent")
			return guardGet(entity.ContainsComponent<MaterialComponent>(), [&]{ return sol::make_object(state, &entity.GetComponent<MaterialComponent>()); });
		if (componentTypeStr == "CameraComponent")
			return guardGet(entity.ContainsComponent<CameraComponent>(), [&]{ return sol::make_object(state, &entity.GetComponent<CameraComponent>()); });
		if (componentTypeStr == "DirectionalLightComponent")
			return guardGet(entity.ContainsComponent<DirectionalLightComponent>(), [&]{ return sol::make_object(state, &entity.GetComponent<DirectionalLightComponent>()); });
		if (componentTypeStr == "SpotLightComponent")
			return guardGet(entity.ContainsComponent<SpotLightComponent>(), [&]{ return sol::make_object(state, &entity.GetComponent<SpotLightComponent>()); });
		if (componentTypeStr == "PointLightComponent")
			return guardGet(entity.ContainsComponent<PointLightComponent>(), [&]{ return sol::make_object(state, &entity.GetComponent<PointLightComponent>()); });
		if (componentTypeStr == "OutlineComponent")
			return guardGet(entity.ContainsComponent<OutlineComponent>(), [&]{ return sol::make_object(state, &entity.GetComponent<OutlineComponent>()); });
		if (componentTypeStr == "BillboardComponent")
			return guardGet(entity.ContainsComponent<BillboardComponent>(), [&]{ return sol::make_object(state, &entity.GetComponent<BillboardComponent>()); });
		if (componentTypeStr == "AnimatorComponent")
			return guardGet(entity.ContainsComponent<AnimatorComponent>(), [&]{ return sol::make_object(state, &entity.GetComponent<AnimatorComponent>()); });
		if (componentTypeStr == "CharacterControllerComponent")
			return guardGet(entity.ContainsComponent<CharacterControllerComponent>(), [&]{ return sol::make_object(state, &entity.GetComponent<CharacterControllerComponent>()); });

		if (componentTypeStr == "ScriptComponent")
		{
			EB_CORE_ASSERT(false, "Cannot get script components from Lua!");
			return sol::lua_nil;
		}

		EB_CORE_ASSERT(false, "Unknown component type: {}", componentTypeStr);
		return sol::lua_nil;
	}

	static sol::object ContainsComponentFromString(const std::string& componentTypeStr, Entity& entity, sol::state& state)
	{
		if (componentTypeStr == "IDComponent")
			return sol::make_object(state, entity.ContainsComponent<IDComponent>());
		if (componentTypeStr == "TagComponent")
			return sol::make_object(state, entity.ContainsComponent<TagComponent>());
		if (componentTypeStr == "RelationshipComponent")
			return sol::make_object(state, entity.ContainsComponent<RelationshipComponent>());
		if (componentTypeStr == "TransformComponent")
			return sol::make_object(state, entity.ContainsComponent<TransformComponent>());
		if (componentTypeStr == "RigidBodyComponent")
			return sol::make_object(state, entity.ContainsComponent<RigidBodyComponent>());
		if (componentTypeStr == "SpriteComponent")
			return sol::make_object(state, entity.ContainsComponent<SpriteComponent>());
		if (componentTypeStr == "StaticMeshComponent")
			return sol::make_object(state, entity.ContainsComponent<StaticMeshComponent>());
		if (componentTypeStr == "SkinnedMeshComponent")
			return sol::make_object(state, entity.ContainsComponent<SkinnedMeshComponent>());
		if (componentTypeStr == "MaterialComponent")
			return sol::make_object(state, entity.ContainsComponent<MaterialComponent>());
		if (componentTypeStr == "CameraComponent")
			return sol::make_object(state, entity.ContainsComponent<CameraComponent>());
		if (componentTypeStr == "DirectionalLightComponent")
			return sol::make_object(state, entity.ContainsComponent<DirectionalLightComponent>());
		if (componentTypeStr == "SpotLightComponent")
			return sol::make_object(state, entity.ContainsComponent<SpotLightComponent>());
		if (componentTypeStr == "PointLightComponent")
			return sol::make_object(state, entity.ContainsComponent<PointLightComponent>());
		if (componentTypeStr == "OutlineComponent")
			return sol::make_object(state, entity.ContainsComponent<OutlineComponent>());
		if (componentTypeStr == "BillboardComponent")
			return sol::make_object(state, entity.ContainsComponent<BillboardComponent>());
		if (componentTypeStr == "AnimatorComponent")
			return sol::make_object(state, entity.ContainsComponent<AnimatorComponent>());
		if (componentTypeStr == "CharacterControllerComponent")
			return sol::make_object(state, entity.ContainsComponent<CharacterControllerComponent>());

		if (componentTypeStr == "ScriptComponent")
		{
			EB_CORE_ASSERT(false, "Cannot check for script components from Lua!");
			return sol::lua_nil;
		}

		EB_CORE_ASSERT(false, "Unknown component type: {}", componentTypeStr);
		return sol::lua_nil;
	}

	void BindEntity(sol::state& state)
	{
		state.new_usertype<Entity>("Entity",
			"GetName", &Entity::GetName,
			"GetUUID", &Entity::GetUUID,
			"GetComponent", [&state](Entity& e, const std::string& componentTypeStr) { return GetComponentFromString(componentTypeStr, e, state); },
				"ContainsComponent", [&state](Entity& e, const std::string& componentTypeStr) { return ContainsComponentFromString(componentTypeStr, e, state); }
		);
	}

}
