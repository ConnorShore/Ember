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
		if (componentTypeStr == "AnimatorComponent")
			return guardGet(entity.ContainsComponent<AnimatorComponent>(), [&]{ return sol::make_object(state, &entity.GetComponent<AnimatorComponent>()); });
		if (componentTypeStr == "BoneSocketComponent")
			return guardGet(entity.ContainsComponent<BoneSocketComponent>(), [&] { return sol::make_object(state, &entity.GetComponent<BoneSocketComponent>()); });
		if (componentTypeStr == "CharacterControllerComponent")
			return guardGet(entity.ContainsComponent<CharacterControllerComponent>(), [&]{ return sol::make_object(state, &entity.GetComponent<CharacterControllerComponent>()); });
		if (componentTypeStr == "TextComponent")
			return guardGet(entity.ContainsComponent<TextComponent>(), [&] { return sol::make_object(state, &entity.GetComponent<TextComponent>()); });
		if (componentTypeStr == "AudioSourceComponent")
			return guardGet(entity.ContainsComponent<AudioSourceComponent>(), [&] { return sol::make_object(state, &entity.GetComponent<AudioSourceComponent>()); });
		if (componentTypeStr == "WaypointComponent")
			return guardGet(entity.ContainsComponent<WaypointComponent>(), [&] { return sol::make_object(state, &entity.GetComponent<WaypointComponent>()); });
		if (componentTypeStr == "AIPathComponent")
			return guardGet(entity.ContainsComponent<AIPathComponent>(), [&] { return sol::make_object(state, &entity.GetComponent<AIPathComponent>()); });
		if (componentTypeStr == "AIAgentComponent")
			return guardGet(entity.ContainsComponent<AIAgentComponent>(), [&] { return sol::make_object(state, &entity.GetComponent<AIAgentComponent>()); });
		if (componentTypeStr == "LocalAvoidanceComponent")
			return guardGet(entity.ContainsComponent<LocalAvoidanceComponent>(), [&] { return sol::make_object(state, &entity.GetComponent<LocalAvoidanceComponent>()); });
		if (componentTypeStr == "ParticleEmitterComponent")
			return guardGet(entity.ContainsComponent<ParticleEmitterComponent>(), [&] { return sol::make_object(state, &entity.GetComponent<ParticleEmitterComponent>()); });
		if (componentTypeStr == "PrefabComponent")
			return guardGet(entity.ContainsComponent<PrefabComponent>(), [&] { return sol::make_object(state, &entity.GetComponent<PrefabComponent>()); });

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
		if (componentTypeStr == "AnimatorComponent")
			return sol::make_object(state, entity.ContainsComponent<AnimatorComponent>());
		if (componentTypeStr == "BoneSocketComponent")
			return sol::make_object(state, entity.ContainsComponent<BoneSocketComponent>());
		if (componentTypeStr == "CharacterControllerComponent")
			return sol::make_object(state, entity.ContainsComponent<CharacterControllerComponent>());
		if (componentTypeStr == "TextComponent")
			return sol::make_object(state, entity.ContainsComponent<TextComponent>());
		if (componentTypeStr == "AudioSourceComponent")
			return sol::make_object(state, entity.ContainsComponent<AudioSourceComponent>());
		if (componentTypeStr == "WaypointComponent")
			return sol::make_object(state, entity.ContainsComponent<WaypointComponent>());
		if (componentTypeStr == "AIPathComponent")
			return sol::make_object(state, entity.ContainsComponent<AIPathComponent>());
		if (componentTypeStr == "AIAgentComponent")
			return sol::make_object(state, entity.ContainsComponent<AIAgentComponent>());
		if (componentTypeStr == "LocalAvoidanceComponent")
			return sol::make_object(state, entity.ContainsComponent<LocalAvoidanceComponent>());
		if (componentTypeStr == "ParticleEmitterComponent")
			return sol::make_object(state, entity.ContainsComponent<ParticleEmitterComponent>());
		if (componentTypeStr == "PrefabComponent")
			return sol::make_object(state, entity.ContainsComponent<PrefabComponent>());

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
				auto& newComp = entity.AttachComponent<ComponentType>();
				return sol::make_object(state, &newComp);
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
		if (componentTypeStr == "AnimatorComponent") return addAndReturn(AnimatorComponent{});
		if (componentTypeStr == "BoneSocketComponent") return addAndReturn(BoneSocketComponent{});
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
		if (componentTypeStr == "ParticleEmitterComponent") return addAndReturn(ParticleEmitterComponent{});
		if (componentTypeStr == "AudioSourceComponent") return addAndReturn(AudioSourceComponent{});
		if (componentTypeStr == "WaypointComponent") return addAndReturn(WaypointComponent{});
		if (componentTypeStr == "AIPathComponent") return addAndReturn(AIPathComponent{});
		if (componentTypeStr == "AIAgentComponent") return addAndReturn(AIAgentComponent{});
		if (componentTypeStr == "LocalAvoidanceComponent") return addAndReturn(LocalAvoidanceComponent{});
		
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
		if (componentTypeStr == "AnimatorComponent") return entity.DetachComponent<AnimatorComponent>();
		if (componentTypeStr == "BoneSocketComponent") return entity.DetachComponent<BoneSocketComponent>();
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
		if (componentTypeStr == "ParticleEmitterComponent") return entity.DetachComponent<ParticleEmitterComponent>();
		if (componentTypeStr == "AudioSourceComponent") return entity.DetachComponent<AudioSourceComponent>();
		if (componentTypeStr == "WaypointComponent") return entity.DetachComponent<WaypointComponent>();
		if (componentTypeStr == "AIPathComponent") return entity.DetachComponent<AIPathComponent>();
		if (componentTypeStr == "AIAgentComponent") return entity.DetachComponent<AIAgentComponent>();
		if (componentTypeStr == "LocalAvoidanceComponent") return entity.DetachComponent<LocalAvoidanceComponent>();

		EB_CORE_ASSERT(false, "Failed to detach component. Unknown component type: {}", componentTypeStr);
	}

	void BindEntity(sol::state& state)
	{
		auto entityType = state.new_usertype<Entity>("Entity",
			"IsValid", [](const Entity& e) { return static_cast<bool>(e); },
			"GetName", &Entity::GetName,
			"GetUUID", &Entity::GetUUID,
			"SetActive", sol::overload(
				[](Entity& e, bool active) { e.SetActive(active, true); },
				[](Entity& e, bool active, bool recursive) { e.SetActive(active, recursive); }
			),
			"IsActive", &Entity::IsActive,
			"AttachComponent", [&state](Entity& e, const std::string& componentTypeStr) { return AddComponentFromString(componentTypeStr, e, state); },
			"DetachComponent", [](Entity& e, const std::string& componentTypeStr) { DetachComponentFromString(componentTypeStr, e); },
			"GetComponent", [&state](Entity& e, const std::string& componentTypeStr) { return GetComponentFromString(componentTypeStr, e, state); },
			"ContainsComponent", [&state](Entity& e, const std::string& componentTypeStr) { return ContainsComponentFromString(componentTypeStr, e, state); },
			"GetParent", &Entity::GetParent,
			"IsRootParent", &Entity::IsRootParent,
			"GetRootParent", &Entity::GetRootParent
		);

		entityType["GetChildren"] = &Entity::GetAllChildren;
		entityType["GetChild"] = &Entity::GetChildByName;
		entityType["AddChild"] = sol::overload(
			static_cast<Entity(Entity::*)(Entity)>(&Entity::AddChild),
			static_cast<Entity(Entity::*)(Entity, bool)>(&Entity::AddChild),
			static_cast<Entity(Entity::*)(const std::string&)>(&Entity::AddChild)
		);

		entityType["GetScriptInstance"] = sol::overload(
			[](Entity& entity, sol::this_state s) -> sol::object {
				if (entity.ContainsComponent<ScriptComponent>())
				{
					auto& scriptComp = entity.GetComponent<ScriptComponent>();
					if (scriptComp.Instance.valid())
					{
						return scriptComp.Instance;
					}
				}
				return sol::make_object(s, sol::lua_nil);
			},
			[](Entity& entity, sol::this_state s, const std::string& scriptName) -> sol::object {
				if (entity.ContainsComponent<ScriptComponent>())
				{
					auto& scriptComp = entity.GetComponent<ScriptComponent>();
					if (scriptComp.Instance.valid())
					{
						sol::table instanceTable = scriptComp.Instance;
						sol::optional<std::string> actualName = instanceTable["__name"];
						if (actualName && actualName.value() == scriptName)
						{
							return scriptComp.Instance;
						}
					}
				}
				return sol::make_object(s, sol::lua_nil);
			}
		);
	}

}
