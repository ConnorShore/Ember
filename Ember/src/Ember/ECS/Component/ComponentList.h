#pragma once

// The single list of components that participate in serialization, component ordering and entity
// resets. Anything added here is picked up by every X-macro expansion that walks it.
#define EMBER_FOR_EACH_COMPONENT_ORDER_TYPE(X) \
	X(IDComponent) \
	X(TagComponent) \
	X(RelationshipComponent) \
	X(TransformComponent) \
	X(RigidBodyComponent) \
	X(BoxColliderComponent) \
	X(SphereColliderComponent) \
	X(CapsuleColliderComponent) \
	X(ConvexMeshColliderComponent) \
	X(ConcaveMeshColliderComponent) \
	X(CharacterControllerComponent) \
	X(SpriteComponent) \
	X(StaticMeshComponent) \
	X(SkinnedMeshComponent) \
	X(MaterialComponent) \
	X(CameraComponent) \
	X(DirectionalLightComponent) \
	X(SpotLightComponent) \
	X(PointLightComponent) \
	X(ScriptComponent) \
	X(OutlineComponent) \
	X(EditorIconComponent) \
	X(AnimatorComponent) \
	X(BoneSocketComponent) \
	X(PrefabComponent) \
	X(LifetimeComponent) \
	X(TextComponent) \
	X(DisabledComponent) \
	X(PoolComponent) \
	X(PoolConfigComponent) \
	X(ParticleEmitterComponent) \
	X(PostProcessVolumeComponent) \
	X(AudioSourceComponent) \
	X(SingleSoundComponent) \
	X(AudioListenerComponent) \
	X(WaypointComponent) \
	X(AIAgentComponent) \
	X(AIPathComponent) \
	X(NavigationGridComponent) \
	X(NavigationMeshComponent) \
	X(NavigationMeshModifierComponent) \
	X(LocalAvoidanceComponent) \
	X(CanvasComponent) \
	X(RectTransformComponent) \
	X(UISelectableComponent) \
	X(UIButtonComponent) \
	X(UIToggleComponent)
