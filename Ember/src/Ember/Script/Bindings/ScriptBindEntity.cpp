#include "ebpch.h"
#include "ScriptBindEntity.h"

#include "Ember/ECS/Component/Components.h"
#include "Ember/Scene/Scene.h"

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
		if (componentTypeStr == "TextComponent")
			return guardGet(entity.ContainsComponent<TextComponent>(), [&] { return sol::make_object(state, &entity.GetComponent<TextComponent>()); });

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
		if (componentTypeStr == "TextComponent")
			return sol::make_object(state, entity.ContainsComponent<TextComponent>());

		if (componentTypeStr == "DisabledComponent")
		{
			EB_CORE_ASSERT(false, "Cannot check for script components from Lua. Use SetActive(bool) method!");
			return sol::lua_nil;
		}
		if (componentTypeStr == "ScriptComponent")
		{
			EB_CORE_ASSERT(false, "Cannot check for script components from Lua!");
			return sol::lua_nil;
		}

		EB_CORE_ASSERT(false, "Unknown component type: {}", componentTypeStr);
		return sol::lua_nil;
	}

	static sol::object AddComponentFromString(const std::string& componentTypeStr, Entity& entity, sol::state& state)
	{
		// Helper lambda that deduces the component type, attaches a blank one, and returns it.
		auto addAndReturn = [&](auto dummyType) -> sol::object
			{
				using ComponentType = decltype(dummyType);

				// Don't add it twice! Just return the existing one if they call it again.
				if (entity.ContainsComponent<ComponentType>())
				{
					EB_CORE_WARN("Entity '{}' already has component '{}'!", entity.GetName(), componentTypeStr);
					return sol::make_object(state, &entity.GetComponent<ComponentType>());
				}

				// Create a new blank component
				ComponentType newComp;
				entity.AttachComponent(newComp);
				return sol::make_object(state, &entity.GetComponent<ComponentType>());
			};

		// Pass a default-constructed instance to deduce the type
		if (componentTypeStr == "TransformComponent") return addAndReturn(TransformComponent{});
		if (componentTypeStr == "RigidBodyComponent") return addAndReturn(RigidBodyComponent{});
		if (componentTypeStr == "SpriteComponent") return addAndReturn(SpriteComponent{});
		if (componentTypeStr == "TextComponent") return addAndReturn(TextComponent{});
		if (componentTypeStr == "CameraComponent") return addAndReturn(CameraComponent{});
		if (componentTypeStr == "PointLightComponent") return addAndReturn(PointLightComponent{});
		if (componentTypeStr == "DirectionalLightComponent") return addAndReturn(DirectionalLightComponent{});
		if (componentTypeStr == "SpotLightComponent") return addAndReturn(SpotLightComponent{});
		if (componentTypeStr == "OutlineComponent") return addAndReturn(OutlineComponent{});
		if (componentTypeStr == "BillboardComponent") return addAndReturn(BillboardComponent{});
		if (componentTypeStr == "AnimatorComponent") return addAndReturn(AnimatorComponent{});
		if (componentTypeStr == "CharacterControllerComponent") return addAndReturn(CharacterControllerComponent{});
		if (componentTypeStr == "StaticMeshComponent") return addAndReturn(StaticMeshComponent{});
		if (componentTypeStr == "SkinnedMeshComponent") return addAndReturn(SkinnedMeshComponent{});
		if (componentTypeStr == "MaterialComponent") return addAndReturn(MaterialComponent{});
		if (componentTypeStr == "RigidBodyComponent") return addAndReturn(RigidBodyComponent{});
		if (componentTypeStr == "BoxColliderComponent") return addAndReturn(BoxColliderComponent{});
		if (componentTypeStr == "SphereColliderComponent") return addAndReturn(SphereColliderComponent{});
		if (componentTypeStr == "CapsuleColliderComponent") return addAndReturn(CapsuleColliderComponent{});
		if (componentTypeStr == "ConcaveMeshColliderComponent") return addAndReturn(ConcaveMeshColliderComponent{});
		if (componentTypeStr == "ConvexMeshColliderComponent") return addAndReturn(ConvexMeshColliderComponent{});
		if (componentTypeStr == "TextComponent") return addAndReturn(TextComponent{});
		if (componentTypeStr == "LifetimeComponent") return addAndReturn(LifetimeComponent{});
		
		if (componentTypeStr == "DisabledComponent")
		{
			EB_CORE_ASSERT(false, "Cannot add script components from Lua. Use SetActive(bool) method!");
			return sol::lua_nil;
		}
		if (componentTypeStr == "ScriptComponent")
		{
			EB_CORE_ASSERT(false, "Cannot add script components from Lua!");
			return sol::lua_nil;
		}

		EB_CORE_ASSERT(false, "Unknown component type: {}", componentTypeStr);
		return sol::lua_nil;
	}

	void DetachComponentFromString(const std::string& componentTypeStr, Entity& entity)
	{
		if (componentTypeStr == "TransformComponent") return entity.DetachComponent<TransformComponent>();
		if (componentTypeStr == "RigidBodyComponent") return entity.DetachComponent<RigidBodyComponent>();
		if (componentTypeStr == "SpriteComponent") return entity.DetachComponent<SpriteComponent>();
		if (componentTypeStr == "TextComponent") return entity.DetachComponent<TextComponent>();
		if (componentTypeStr == "CameraComponent") return entity.DetachComponent<CameraComponent>();
		if (componentTypeStr == "PointLightComponent") return entity.DetachComponent<PointLightComponent>();
		if (componentTypeStr == "DirectionalLightComponent") return entity.DetachComponent<DirectionalLightComponent>();
		if (componentTypeStr == "SpotLightComponent") return entity.DetachComponent<SpotLightComponent>();
		if (componentTypeStr == "OutlineComponent") return entity.DetachComponent<OutlineComponent>();
		if (componentTypeStr == "BillboardComponent") return entity.DetachComponent<BillboardComponent>();
		if (componentTypeStr == "AnimatorComponent") return entity.DetachComponent<AnimatorComponent>();
		if (componentTypeStr == "CharacterControllerComponent") return entity.DetachComponent<CharacterControllerComponent>();
		if (componentTypeStr == "StaticMeshComponent") return entity.DetachComponent<StaticMeshComponent>();
		if (componentTypeStr == "SkinnedMeshComponent") return entity.DetachComponent<SkinnedMeshComponent>();
		if (componentTypeStr == "MaterialComponent") return entity.DetachComponent<MaterialComponent>();
		if (componentTypeStr == "RigidBodyComponent") return entity.DetachComponent<RigidBodyComponent>();
		if (componentTypeStr == "BoxColliderComponent") return entity.DetachComponent<BoxColliderComponent>();
		if (componentTypeStr == "SphereColliderComponent") return entity.DetachComponent<SphereColliderComponent>();
		if (componentTypeStr == "CapsuleColliderComponent") return entity.DetachComponent<CapsuleColliderComponent>();
		if (componentTypeStr == "ConcaveMeshColliderComponent") return entity.DetachComponent<ConcaveMeshColliderComponent>();
		if (componentTypeStr == "ConvexMeshColliderComponent") return entity.DetachComponent<ConvexMeshColliderComponent>();
		if (componentTypeStr == "TextComponent") return entity.DetachComponent<TextComponent>();
		if (componentTypeStr == "LifetimeComponent") return entity.DetachComponent<LifetimeComponent>();
		if (componentTypeStr == "ScriptComponent") return entity.DetachComponent<ScriptComponent>();

		EB_CORE_ASSERT(false, "Failed to detach component. Unknown component type: {}", componentTypeStr);
	}

	void BindEntity(sol::state& state)
	{
		auto entityType = state.new_usertype<Entity>("Entity",
			"GetName", &Entity::GetName,
			"GetUUID", &Entity::GetUUID,
			"SetActive", [&state](Entity& e, bool active) { if (active) e.DetachComponent<DisabledComponent>(); else { DisabledComponent dc; e.AttachComponent(dc); }},
			"AttachComponent", [&state](Entity& e, const std::string& componentTypeStr) { return AddComponentFromString(componentTypeStr, e, state); },
			"DetachComponent", [](Entity& e, const std::string& componentTypeStr) { DetachComponentFromString(componentTypeStr, e); },
			"GetComponent", [&state](Entity& e, const std::string& componentTypeStr) { return GetComponentFromString(componentTypeStr, e, state); },
			"ContainsComponent", [&state](Entity& e, const std::string& componentTypeStr) { return ContainsComponentFromString(componentTypeStr, e, state); }
		);

		entityType.set_function("GetScriptInstance", [](Entity& entity, sol::this_state s) -> sol::object {
			if (entity.ContainsComponent<ScriptComponent>())
			{
				auto& scriptComp = entity.GetComponent<ScriptComponent>();
				if (scriptComp.Instance.valid())
				{
					return scriptComp.Instance;
				}
			}
			return sol::make_object(s, sol::lua_nil);
		});
	}

}
