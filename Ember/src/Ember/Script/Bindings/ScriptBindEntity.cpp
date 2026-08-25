#include "ebpch.h"
#include "ScriptBindEntity.h"

#include "Ember/ECS/Component/Components.h"
#include "Ember/Scene/Scene.h"
#include "Ember/Script/Bindings/ScriptComponentRef.h"
#include "Ember/Script/ScriptEngine.h"

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
			return guardGet(entity.ContainsComponent<IDComponent>(), [&]{ return PushComponent<IDComponent>(state, entity); });
		if (componentTypeStr == "TagComponent")
			return guardGet(entity.ContainsComponent<TagComponent>(), [&]{ return PushComponent<TagComponent>(state, entity); });
		if (componentTypeStr == "RelationshipComponent")
			return guardGet(entity.ContainsComponent<RelationshipComponent>(), [&]{ return PushComponent<RelationshipComponent>(state, entity); });
		if (componentTypeStr == "TransformComponent")
			return guardGet(entity.ContainsComponent<TransformComponent>(), [&]{ return PushComponent<TransformComponent>(state, entity); });
		if (componentTypeStr == "RigidBodyComponent")
			return guardGet(entity.ContainsComponent<RigidBodyComponent>(), [&]{ return PushComponent<RigidBodyComponent>(state, entity); });
		if (componentTypeStr == "SpriteComponent")
			return guardGet(entity.ContainsComponent<SpriteComponent>(), [&]{ return PushComponent<SpriteComponent>(state, entity); });
		if (componentTypeStr == "StaticMeshComponent")
			return guardGet(entity.ContainsComponent<StaticMeshComponent>(), [&]{ return PushComponent<StaticMeshComponent>(state, entity); });
		if (componentTypeStr == "SkinnedMeshComponent")
			return guardGet(entity.ContainsComponent<SkinnedMeshComponent>(), [&]{ return PushComponent<SkinnedMeshComponent>(state, entity); });
		if (componentTypeStr == "MaterialComponent")
			return guardGet(entity.ContainsComponent<MaterialComponent>(), [&]{ return PushComponent<MaterialComponent>(state, entity); });
		if (componentTypeStr == "CameraComponent")
			return guardGet(entity.ContainsComponent<CameraComponent>(), [&]{ return PushComponent<CameraComponent>(state, entity); });
		if (componentTypeStr == "DirectionalLightComponent")
			return guardGet(entity.ContainsComponent<DirectionalLightComponent>(), [&]{ return PushComponent<DirectionalLightComponent>(state, entity); });
		if (componentTypeStr == "SpotLightComponent")
			return guardGet(entity.ContainsComponent<SpotLightComponent>(), [&]{ return PushComponent<SpotLightComponent>(state, entity); });
		if (componentTypeStr == "PointLightComponent")
			return guardGet(entity.ContainsComponent<PointLightComponent>(), [&]{ return PushComponent<PointLightComponent>(state, entity); });
		if (componentTypeStr == "OutlineComponent")
			return guardGet(entity.ContainsComponent<OutlineComponent>(), [&]{ return PushComponent<OutlineComponent>(state, entity); });
		if (componentTypeStr == "AnimatorComponent")
			return guardGet(entity.ContainsComponent<AnimatorComponent>(), [&]{ return PushComponent<AnimatorComponent>(state, entity); });
		if (componentTypeStr == "BoneSocketComponent")
			return guardGet(entity.ContainsComponent<BoneSocketComponent>(), [&] { return PushComponent<BoneSocketComponent>(state, entity); });
		if (componentTypeStr == "CharacterControllerComponent")
			return guardGet(entity.ContainsComponent<CharacterControllerComponent>(), [&]{ return PushComponent<CharacterControllerComponent>(state, entity); });
		if (componentTypeStr == "TextComponent")
			return guardGet(entity.ContainsComponent<TextComponent>(), [&] { return PushComponent<TextComponent>(state, entity); });
		if (componentTypeStr == "CanvasComponent")
			return guardGet(entity.ContainsComponent<CanvasComponent>(), [&] { return PushComponent<CanvasComponent>(state, entity); });
		if (componentTypeStr == "RectTransformComponent")
			return guardGet(entity.ContainsComponent<RectTransformComponent>(), [&] { return PushComponent<RectTransformComponent>(state, entity); });
		if (componentTypeStr == "UISelectableComponent")
			return guardGet(entity.ContainsComponent<UISelectableComponent>(), [&] { return PushComponent<UISelectableComponent>(state, entity); });
		if (componentTypeStr == "UIButtonComponent")
			return guardGet(entity.ContainsComponent<UIButtonComponent>(), [&] { return PushComponent<UIButtonComponent>(state, entity); });
		if (componentTypeStr == "UIToggleComponent")
			return guardGet(entity.ContainsComponent<UIToggleComponent>(), [&] { return PushComponent<UIToggleComponent>(state, entity); });
		if (componentTypeStr == "AudioSourceComponent")
			return guardGet(entity.ContainsComponent<AudioSourceComponent>(), [&] { return PushComponent<AudioSourceComponent>(state, entity); });
		if (componentTypeStr == "WaypointComponent")
			return guardGet(entity.ContainsComponent<WaypointComponent>(), [&] { return PushComponent<WaypointComponent>(state, entity); });
		if (componentTypeStr == "AIPathComponent")
			return guardGet(entity.ContainsComponent<AIPathComponent>(), [&] { return PushComponent<AIPathComponent>(state, entity); });
		if (componentTypeStr == "AIAgentComponent")
			return guardGet(entity.ContainsComponent<AIAgentComponent>(), [&] { return PushComponent<AIAgentComponent>(state, entity); });
		if (componentTypeStr == "LocalAvoidanceComponent")
			return guardGet(entity.ContainsComponent<LocalAvoidanceComponent>(), [&] { return PushComponent<LocalAvoidanceComponent>(state, entity); });
		if (componentTypeStr == "ParticleEmitterComponent")
			return guardGet(entity.ContainsComponent<ParticleEmitterComponent>(), [&] { return PushComponent<ParticleEmitterComponent>(state, entity); });
		if (componentTypeStr == "PrefabComponent")
			return guardGet(entity.ContainsComponent<PrefabComponent>(), [&] { return PushComponent<PrefabComponent>(state, entity); });

		if (componentTypeStr == "ScriptComponent")
		{
			EB_CORE_ERROR("Cannot get script components from Lua!");
			return sol::lua_nil;
		}

		EB_CORE_ERROR("Unknown component type: {}", componentTypeStr);
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
		if (componentTypeStr == "CanvasComponent")
			return sol::make_object(state, entity.ContainsComponent<CanvasComponent>());
		if (componentTypeStr == "RectTransformComponent")
			return sol::make_object(state, entity.ContainsComponent<RectTransformComponent>());
		if (componentTypeStr == "UISelectableComponent")
			return sol::make_object(state, entity.ContainsComponent<UISelectableComponent>());
		if (componentTypeStr == "UIButtonComponent")
			return sol::make_object(state, entity.ContainsComponent<UIButtonComponent>());
		if (componentTypeStr == "UIToggleComponent")
			return sol::make_object(state, entity.ContainsComponent<UIToggleComponent>());
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
			EB_CORE_ERROR("Cannot check for script components from Lua. Use SetActive(bool) method!");
			return sol::lua_nil;
		}
		if (componentTypeStr == "ScriptComponent")
		{
			EB_CORE_ERROR("Cannot check for script components from Lua!");
			return sol::lua_nil;
		}

		EB_CORE_ERROR("Unknown component type: {}", componentTypeStr);
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
					return PushComponent<ComponentType>(state, entity);
				}

				// Create a new blank component
				auto& newComp = entity.AttachComponent<ComponentType>();
				return PushComponent<ComponentType>(state, entity);
			};

		// Pass a default-constructed instance to deduce the type
		if (componentTypeStr == "TransformComponent") return addAndReturn(TransformComponent{});
		if (componentTypeStr == "RigidBodyComponent") return addAndReturn(RigidBodyComponent{});
		if (componentTypeStr == "SpriteComponent") return addAndReturn(SpriteComponent{});
		if (componentTypeStr == "TextComponent") return addAndReturn(TextComponent{});
		if (componentTypeStr == "CanvasComponent") return addAndReturn(CanvasComponent{});
		if (componentTypeStr == "RectTransformComponent") return addAndReturn(RectTransformComponent{});
		if (componentTypeStr == "UISelectableComponent") return addAndReturn(UISelectableComponent{});
		if (componentTypeStr == "UIButtonComponent") return addAndReturn(UIButtonComponent{});
		if (componentTypeStr == "UIToggleComponent") return addAndReturn(UIToggleComponent{});
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
		if (componentTypeStr == "CanvasComponent") return addAndReturn(CanvasComponent{});
		if (componentTypeStr == "RectTransformComponent") return addAndReturn(RectTransformComponent{});
		if (componentTypeStr == "UISelectableComponent") return addAndReturn(UISelectableComponent{});
		if (componentTypeStr == "UIButtonComponent") return addAndReturn(UIButtonComponent{});
		if (componentTypeStr == "UIToggleComponent") return addAndReturn(UIToggleComponent{});
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
		if (componentTypeStr == "CanvasComponent") return entity.DetachComponent<CanvasComponent>();
		if (componentTypeStr == "RectTransformComponent") return entity.DetachComponent<RectTransformComponent>();
		if (componentTypeStr == "UISelectableComponent") return entity.DetachComponent<UISelectableComponent>();
		if (componentTypeStr == "UIButtonComponent") return entity.DetachComponent<UIButtonComponent>();
		if (componentTypeStr == "UIToggleComponent") return entity.DetachComponent<UIToggleComponent>();
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
		if (componentTypeStr == "CanvasComponent") return entity.DetachComponent<CanvasComponent>();
		if (componentTypeStr == "RectTransformComponent") return entity.DetachComponent<RectTransformComponent>();
		if (componentTypeStr == "UISelectableComponent") return entity.DetachComponent<UISelectableComponent>();
		if (componentTypeStr == "UIButtonComponent") return entity.DetachComponent<UIButtonComponent>();
		if (componentTypeStr == "UIToggleComponent") return entity.DetachComponent<UIToggleComponent>();
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
			"GetID", & Entity::GetEntityHandle,
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

						// Not the concrete script's own name - check whether it matches something
						// in the script's Base ancestry instead, so e.g. GetScriptInstance("PurchasableItem")
						// finds an entity whose attached script is PickupItem (Base = "PurchasableItem").
						// Indexed access rather than a key/value iterator: __baseChain is always a
						// dense 1-indexed array we built ourselves, so there's no need for pairs()-style
						// iteration here.
						sol::optional<sol::table> baseChain = instanceTable[ScriptEngine::BaseChainFieldName];
						if (baseChain)
						{
							sol::table chain = baseChain.value();
							size_t chainLength = chain.size();
							for (size_t i = 1; i <= chainLength; ++i)
							{
								sol::optional<std::string> ancestorName = chain[i];
								if (ancestorName && ancestorName.value() == scriptName)
									return scriptComp.Instance;
							}
						}
					}
				}
				return sol::make_object(s, sol::lua_nil);
			}
		);
	}

}
