#include "ebpch.h"
#include "SceneSerializer.h"

#include "Ember/ECS/Component/Components.h"
#include "Ember/Utils/SerializationUtils.h"
#include "Ember/Core/Application.h"
#include "Ember/ECS/System/RenderSystem.h"
#include "Ember/Render/VFX/BloomPass.h"
#include "Ember/Render/VFX/FXAAPass.h"
#include "Ember/Render/VFX/ColorGradePass.h"
#include "Ember/Render/VFX/ToneMapPass.h"
#include "Ember/Render/VFX/FogPass.h"
#include "Ember/Render/VFX/VignettePass.h"

#include <ryml.hpp>
#include <ryml_std.hpp>
#include <string>
#include <fstream>
#include <sstream>
#include <queue>
#include <vector>

namespace Ember {

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
	X(LocalAvoidanceComponent) \
	X(CanvasComponent) \
	X(RectTransformComponent)

	template<typename T>
	static bool TrySerializeComponentOrderEntry(Entity entity, ComponentType componentType, ryml::NodeRef& componentOrderNode, const char* componentName)
	{
		if (entity.GetComponentType<T>() != componentType)
			return false;

		if (entity.ContainsComponent<T>())
			componentOrderNode.append_child() << std::string(componentName);

		return true;
	}

	static void SerializeComponentOrder(ryml::NodeRef& entityNode, Entity entity)
	{
		ryml::NodeRef componentOrderNode = entityNode["ComponentOrder"];
		componentOrderNode |= ryml::SEQ;

		for (ComponentType componentType : entity.GetComponentOrder())
		{
#define TRY_SERIALIZE_COMPONENT_ORDER_ENTRY(Component) \
			if (TrySerializeComponentOrderEntry<Component>(entity, componentType, componentOrderNode, #Component)) \
				continue;

			EMBER_FOR_EACH_COMPONENT_ORDER_TYPE(TRY_SERIALIZE_COMPONENT_ORDER_ENTRY)

#undef TRY_SERIALIZE_COMPONENT_ORDER_ENTRY
		}
	}

	template<typename T>
	static bool TryDeserializeComponentOrderEntry(Entity entity, const std::string& componentName, std::vector<ComponentType>& componentOrder, const char* expectedComponentName)
	{
		if (componentName != expectedComponentName)
			return false;

		if (entity.ContainsComponent<T>())
			componentOrder.push_back(entity.GetComponentType<T>());

		return true;
	}

	static void DeserializeComponentOrder(ryml::NodeRef& entityNode, Entity entity)
	{
		if (!entityNode.has_child("ComponentOrder"))
			return;

		std::vector<ComponentType> componentOrder;
		for (ryml::NodeRef componentNode : entityNode["ComponentOrder"].children())
		{
			std::string componentName;
			componentNode >> componentName;

#define TRY_DESERIALIZE_COMPONENT_ORDER_ENTRY(Component) \
			if (TryDeserializeComponentOrderEntry<Component>(entity, componentName, componentOrder, #Component)) \
				continue;

			EMBER_FOR_EACH_COMPONENT_ORDER_TYPE(TRY_DESERIALIZE_COMPONENT_ORDER_ENTRY)

#undef TRY_DESERIALIZE_COMPONENT_ORDER_ENTRY
		}

		entity.SetComponentOrder(componentOrder);
	}

#undef EMBER_FOR_EACH_COMPONENT_ORDER_TYPE

	// =========================================================================
	// HELPER: SERIALIZE SINGLE ENTITY
	// =========================================================================
	void SceneSerializer::SerializeEntityNode(ryml::NodeRef& entityNode, Entity entity)
	{
		entityNode |= ryml::MAP;
		entityNode["Entity"] << (uint64_t)entity.GetComponent<IDComponent>().ID;
		SerializeComponentOrder(entityNode, entity);

		if (entity.ContainsComponent<TagComponent>())
		{
			ryml::NodeRef tagNode = entityNode["TagComponent"];
			tagNode |= ryml::MAP;
			tagNode["Tag"] << entity.GetComponent<TagComponent>().Tag;
		}
		if (entity.ContainsComponent<RelationshipComponent>())
		{
			auto& relationship = entity.GetComponent<RelationshipComponent>();
			ryml::NodeRef relationshipNode = entityNode["RelationshipComponent"];
			relationshipNode |= ryml::MAP;
			relationshipNode["Parent"] << (uint64_t)relationship.ParentHandle;
			ryml::NodeRef childrenNode = relationshipNode["Children"];
			childrenNode |= ryml::SEQ;
			for (const auto& child : relationship.Children)
			{
				childrenNode.append_child() << (uint64_t)child;
			}
			relationshipNode["IsAttachment"] << relationship.IsAttachment;
		}
		if (entity.ContainsComponent<TransformComponent>())
		{
			ryml::NodeRef transformNode = entityNode["TransformComponent"];
			transformNode |= ryml::MAP;
			Util::SerializeVector3f(transformNode["Position"], entity.GetComponent<TransformComponent>().Position);
			Util::SerializeVector3f(transformNode["Rotation"], entity.GetComponent<TransformComponent>().Rotation);
			Util::SerializeVector3f(transformNode["Scale"], entity.GetComponent<TransformComponent>().Scale);
			Util::SerializeMatrix4f(transformNode["WorldTransform"], entity.GetComponent<TransformComponent>().WorldTransform);
		}
		if (entity.ContainsComponent<SpriteComponent>())
		{
			auto& sprite = entity.GetComponent<SpriteComponent>();
			ryml::NodeRef spriteNode = entityNode["SpriteComponent"];
			spriteNode |= ryml::MAP;
			Util::SerializeVector4f(spriteNode["Color"], sprite.Color);
			if (sprite.TextureHandle != Constants::InvalidUUID)
			{
				spriteNode["TextureUUID"] << sprite.TextureHandle;
			}
			spriteNode["IsBillboard"] << sprite.IsBillboard;
			spriteNode["LockYAxis"] << sprite.LockYAxis;
		}
		if (entity.ContainsComponent<RigidBodyComponent>())
		{
			ryml::NodeRef rigidBodyNode = entityNode["RigidBodyComponent"];
			rigidBodyNode |= ryml::MAP;

			rigidBodyNode["Type"] << (int)entity.GetComponent<RigidBodyComponent>().Type;
			rigidBodyNode["Mass"] << entity.GetComponent<RigidBodyComponent>().Mass;
			rigidBodyNode["GravityEnabled"] << entity.GetComponent<RigidBodyComponent>().GravityEnabled;
		}
		if (entity.ContainsComponent<BoxColliderComponent>())
		{
			ryml::NodeRef colliderNode = entityNode["BoxColliderComponent"];
			colliderNode |= ryml::MAP;

			Util::SerializeVector3f(colliderNode["Size"], entity.GetComponent<BoxColliderComponent>().Size);
			colliderNode["IsTrigger"] << entity.GetComponent<BoxColliderComponent>().IsTrigger;
			Util::SerializeVector3f(colliderNode["OffsetPosition"], entity.GetComponent<BoxColliderComponent>().Offset.Position);
			Util::SerializeVector3f(colliderNode["OffsetRotation"], entity.GetComponent<BoxColliderComponent>().Offset.Rotation);

			colliderNode["Category"] << entity.GetComponent<BoxColliderComponent>().Category;
			colliderNode["CollisionMask"] << entity.GetComponent<BoxColliderComponent>().CollisionMask;
			colliderNode["PhysicsMaterialUUID"] << entity.GetComponent<BoxColliderComponent>().PhysicsMaterialHandle;
			colliderNode["PreviewCollider"] << entity.GetComponent<BoxColliderComponent>().PreviewCollider;
		}
		if (entity.ContainsComponent<SphereColliderComponent>())
		{
			ryml::NodeRef colliderNode = entityNode["SphereColliderComponent"];
			colliderNode |= ryml::MAP;
			colliderNode["Radius"] << entity.GetComponent<SphereColliderComponent>().Radius;
			colliderNode["IsTrigger"] << entity.GetComponent<SphereColliderComponent>().IsTrigger;
			Util::SerializeVector3f(colliderNode["OffsetPosition"], entity.GetComponent<SphereColliderComponent>().Offset.Position);
			Util::SerializeVector3f(colliderNode["OffsetRotation"], entity.GetComponent<SphereColliderComponent>().Offset.Rotation);

			colliderNode["Category"] << entity.GetComponent<SphereColliderComponent>().Category;
			colliderNode["CollisionMask"] << entity.GetComponent<SphereColliderComponent>().CollisionMask;
			colliderNode["PhysicsMaterialUUID"] << entity.GetComponent<SphereColliderComponent>().PhysicsMaterialHandle;
			colliderNode["PreviewCollider"] << entity.GetComponent<SphereColliderComponent>().PreviewCollider;
		}
		if (entity.ContainsComponent<CapsuleColliderComponent>())
		{
			ryml::NodeRef colliderNode = entityNode["CapsuleColliderComponent"];
			colliderNode |= ryml::MAP;
			colliderNode["Radius"] << entity.GetComponent<CapsuleColliderComponent>().Radius;
			colliderNode["Height"] << entity.GetComponent<CapsuleColliderComponent>().Height;
			colliderNode["IsTrigger"] << entity.GetComponent<CapsuleColliderComponent>().IsTrigger;
			Util::SerializeVector3f(colliderNode["OffsetPosition"], entity.GetComponent<CapsuleColliderComponent>().Offset.Position);
			Util::SerializeVector3f(colliderNode["OffsetRotation"], entity.GetComponent<CapsuleColliderComponent>().Offset.Rotation);

			colliderNode["Category"] << entity.GetComponent<CapsuleColliderComponent>().Category;
			colliderNode["CollisionMask"] << entity.GetComponent<CapsuleColliderComponent>().CollisionMask;
			colliderNode["PhysicsMaterialUUID"] << entity.GetComponent<CapsuleColliderComponent>().PhysicsMaterialHandle;
			colliderNode["PreviewCollider"] << entity.GetComponent<CapsuleColliderComponent>().PreviewCollider;
		}
		if (entity.ContainsComponent<ConvexMeshColliderComponent>())
		{
			ryml::NodeRef colliderNode = entityNode["ConvexMeshColliderComponent"];
			colliderNode |= ryml::MAP;
			if (entity.GetComponent<ConvexMeshColliderComponent>().MeshHandle != Constants::InvalidUUID)
			{
				colliderNode["MeshUUID"] << entity.GetComponent<ConvexMeshColliderComponent>().MeshHandle;

				colliderNode["IsTrigger"] << entity.GetComponent<ConvexMeshColliderComponent>().IsTrigger;
				Util::SerializeVector3f(colliderNode["OffsetPosition"], entity.GetComponent<ConvexMeshColliderComponent>().Offset.Position);
				Util::SerializeVector3f(colliderNode["OffsetRotation"], entity.GetComponent<ConvexMeshColliderComponent>().Offset.Rotation);

				colliderNode["Category"] << entity.GetComponent<ConvexMeshColliderComponent>().Category;
				colliderNode["CollisionMask"] << entity.GetComponent<ConvexMeshColliderComponent>().CollisionMask;
				colliderNode["PhysicsMaterialUUID"] << entity.GetComponent<ConvexMeshColliderComponent>().PhysicsMaterialHandle;
				colliderNode["PreviewCollider"] << entity.GetComponent<ConvexMeshColliderComponent>().PreviewCollider;
			}
		}
		if (entity.ContainsComponent<ConcaveMeshColliderComponent>())
		{
			ryml::NodeRef colliderNode = entityNode["ConcaveMeshColliderComponent"];
			colliderNode |= ryml::MAP;
			if (entity.GetComponent<ConcaveMeshColliderComponent>().MeshHandle != Constants::InvalidUUID)
			{
				colliderNode["MeshUUID"] << entity.GetComponent<ConcaveMeshColliderComponent>().MeshHandle;

				colliderNode["IsTrigger"] << entity.GetComponent<ConcaveMeshColliderComponent>().IsTrigger;
				Util::SerializeVector3f(colliderNode["OffsetPosition"], entity.GetComponent<ConcaveMeshColliderComponent>().Offset.Position);
				Util::SerializeVector3f(colliderNode["OffsetRotation"], entity.GetComponent<ConcaveMeshColliderComponent>().Offset.Rotation);

				colliderNode["Category"] << entity.GetComponent<ConcaveMeshColliderComponent>().Category;
				colliderNode["CollisionMask"] << entity.GetComponent<ConcaveMeshColliderComponent>().CollisionMask;
				colliderNode["PhysicsMaterialUUID"] << entity.GetComponent<ConcaveMeshColliderComponent>().PhysicsMaterialHandle;
				colliderNode["PreviewCollider"] << entity.GetComponent<ConcaveMeshColliderComponent>().PreviewCollider;
			}
		}
		if (entity.ContainsComponent<StaticMeshComponent>())
		{
			ryml::NodeRef meshNode = entityNode["StaticMeshComponent"];
			meshNode |= ryml::MAP;
			auto& staticMeshComp = entity.GetComponent<StaticMeshComponent>();
			if (staticMeshComp.MeshHandle != Constants::InvalidUUID)
			{
				meshNode["MeshUUID"] << (uint64_t)staticMeshComp.MeshHandle;
				meshNode["Layer"] << staticMeshComp.Layer;
			}
		}
		if (entity.ContainsComponent<SkinnedMeshComponent>())
		{
			ryml::NodeRef meshNode = entityNode["SkinnedMeshComponent"];
			meshNode |= ryml::MAP;
			auto& skinnedMeshComp = entity.GetComponent<SkinnedMeshComponent>();
			if (skinnedMeshComp.MeshHandle != Constants::InvalidUUID)
			{
				meshNode["MeshUUID"] << skinnedMeshComp.MeshHandle;
				meshNode["RootAnimator"] << (uint64_t)skinnedMeshComp.AnimatorEntityHandle;
				meshNode["Layer"] << skinnedMeshComp.Layer;
			}
		}
		if (entity.ContainsComponent<MaterialComponent>())
		{
			ryml::NodeRef materialNode = entityNode["MaterialComponent"];
			materialNode |= ryml::MAP;
			auto materialHandle = entity.GetComponent<MaterialComponent>().MaterialHandle;
			if (materialHandle != Constants::InvalidUUID)
			{
				materialNode["MaterialUUID"] << (uint64_t)materialHandle;
			}
		}
		if (entity.ContainsComponent<CameraComponent>())
		{
			auto& cameraComp = entity.GetComponent<CameraComponent>();
			auto& camera = cameraComp.Camera;
			ryml::NodeRef cameraNode = entityNode["CameraComponent"];
			cameraNode |= ryml::MAP;
			Util::SerializeMatrix4f(cameraNode["Projection"], camera.GetProjectionMatrix());
			cameraNode["Type"] << (int)camera.GetProjectionType();
			cameraNode["IsActive"] << cameraComp.IsActive;
			cameraNode["RenderMask"] << cameraComp.RenderMask;
			cameraNode["VolumeMask"] << cameraComp.VolumeMask;

			ryml::NodeRef orthoPropsNode = cameraNode["OrthographicProperties"];
			orthoPropsNode |= ryml::MAP;
			orthoPropsNode["Size"] << camera.GetOrthographicProps().Size;
			orthoPropsNode["NearClip"] << camera.GetOrthographicProps().NearClip;
			orthoPropsNode["FarClip"] << camera.GetOrthographicProps().FarClip;

			ryml::NodeRef perspectivePropsNode = cameraNode["PerspectiveProperties"];
			perspectivePropsNode |= ryml::MAP;
			perspectivePropsNode["FOV"] << camera.GetPerspectiveProps().FieldOfView;
			perspectivePropsNode["NearClip"] << camera.GetPerspectiveProps().NearClip;
			perspectivePropsNode["FarClip"] << camera.GetPerspectiveProps().FarClip;
		}
		if (entity.ContainsComponent<DirectionalLightComponent>())
		{
			auto& lightComp = entity.GetComponent<DirectionalLightComponent>();
			ryml::NodeRef lightNode = entityNode["DirectionalLightComponent"];
			lightNode |= ryml::MAP;
			lightNode["Active"] << lightComp.Active;
			Util::SerializeVector3f(lightNode["Color"], lightComp.Color);
			lightNode["Intensity"] << lightComp.Intensity;
		}
		if (entity.ContainsComponent<SpotLightComponent>())
		{
			auto& lightComp = entity.GetComponent<SpotLightComponent>();
			ryml::NodeRef lightNode = entityNode["SpotLightComponent"];
			lightNode |= ryml::MAP;
			lightNode["Active"] << lightComp.Active;
			Util::SerializeVector3f(lightNode["Color"], lightComp.Color);
			lightNode["Intensity"] << lightComp.Intensity;
			lightNode["CutOffAngle"] << lightComp.CutOffAngle;
			lightNode["OuterCutOffAngle"] << lightComp.OuterCutOffAngle;
		}
		if (entity.ContainsComponent<PointLightComponent>())
		{
			auto& lightComp = entity.GetComponent<PointLightComponent>();
			ryml::NodeRef lightNode = entityNode["PointLightComponent"];
			lightNode |= ryml::MAP;
			lightNode["Active"] << lightComp.Active;
			Util::SerializeVector3f(lightNode["Color"], lightComp.Color);
			lightNode["Intensity"] << lightComp.Intensity;
			lightNode["Radius"] << lightComp.Radius;
		}
		if (entity.ContainsComponent<ScriptComponent>())
		{
			auto& script = entity.GetComponent<ScriptComponent>();

			auto scriptNode = entityNode["ScriptComponent"];
			scriptNode |= ryml::MAP;
			scriptNode["ScriptUUID"] << (uint64_t)script.ScriptHandle;

			ryml::NodeRef overridesNode = scriptNode["UserPropertyOverrides"];
			overridesNode |= ryml::SEQ;

			for (const auto& [name, prop] : script.UserPropertyOverrides)
			{
				auto propNode = overridesNode.append_child();
				propNode |= ryml::MAP;

				propNode["Name"] << name;
				propNode["Type"] << (int)prop.Type;

				switch (prop.Type)
				{
				case ScriptPropertyType::Float:
					propNode["Value"] << std::get<float>(prop.Value);
					break;
				case ScriptPropertyType::Int:
				case ScriptPropertyType::Enum:
					propNode["Value"] << std::get<int>(prop.Value);
					break;
				case ScriptPropertyType::Bool:
					propNode["Value"] << std::get<bool>(prop.Value);
					break;
				case ScriptPropertyType::String:
					propNode["Value"] << std::get<std::string>(prop.Value);
					break;
				case ScriptPropertyType::Vector3f:
					Util::SerializeVector3f(propNode["Value"], std::get<Ember::Vector3f>(prop.Value));
					break;
				case ScriptPropertyType::EntityRef:
				case ScriptPropertyType::AssetRef:
					propNode["Value"] << (uint64_t)std::get<Ember::UUID>(prop.Value);
					propNode["ReferenceKind"] << ScriptReferenceKindToString(prop.ReferenceKind);
					break;
				default:
					break;
				}
			}
		}
		//if (entity.ContainsComponent<OutlineComponent>())
		//{
		//	auto& outlineComp = entity.GetComponent<OutlineComponent>();
		//	ryml::NodeRef outlineNode = entityNode["OutlineComponent"];
		//	outlineNode |= ryml::MAP;
		//	Util::SerializeVector3f(outlineNode["Color"], outlineComp.Color);
		//	outlineNode["Thickness"] << outlineComp.Thickness;
		//}
		if (entity.ContainsComponent<AnimatorComponent>())
		{
			auto& animator = entity.GetComponent<AnimatorComponent>();
			ryml::NodeRef animatorNode = entityNode["AnimatorComponent"];
			animatorNode |= ryml::MAP;
			animatorNode["SkeletonHandle"] << (uint64_t)animator.SkeletonHandle;
			animatorNode["CurrentAnimationHandle"] << (uint64_t)animator.CurrentAnimationHandle;
		}
		if (entity.ContainsComponent<BoneSocketComponent>())
		{
			auto& socket = entity.GetComponent<BoneSocketComponent>();
			ryml::NodeRef socketNode = entityNode["BoneSocketComponent"];
			socketNode |= ryml::MAP;
			socketNode["TargetEntity"] << (uint64_t)socket.TargetEntityHandle;
			socketNode["BoneName"] << socket.BoneName;
			Util::SerializeVector3f(socketNode["Position"], socket.Position);
			Util::SerializeVector3f(socketNode["Rotation"], socket.Rotation);
			Util::SerializeVector3f(socketNode["Scale"], socket.Scale);
		}
		if (entity.ContainsComponent<PrefabComponent>())
		{
			auto& prefab = entity.GetComponent<PrefabComponent>();
			ryml::NodeRef prefabNode = entityNode["PrefabComponent"];
			prefabNode |= ryml::MAP;
			prefabNode["PrefabUUID"] << (uint64_t)prefab.PrefabHandle;
		}
		if (entity.ContainsComponent<CharacterControllerComponent>())
		{
			auto& controller = entity.GetComponent<CharacterControllerComponent>();
			ryml::NodeRef controllerNode = entityNode["CharacterControllerComponent"];
			controllerNode |= ryml::MAP;
			controllerNode["WalkSpeed"] << controller.WalkSpeed;
			controllerNode["JumpForce"] << controller.JumpForce;
			controllerNode["GravityMultiplier"] << controller.GravityMultiplier;
			controllerNode["MaxSlopeAngle"] << controller.MaxSlopeAngle;
			controllerNode["MaxStepHeight"] << controller.MaxStepHeight;
		}
		if (entity.ContainsComponent<LifetimeComponent>())
		{
			auto& lifetime = entity.GetComponent<LifetimeComponent>();
			ryml::NodeRef prefabNode = entityNode["LifetimeComponent"];
			prefabNode |= ryml::MAP;
			prefabNode["Lifetime"] << lifetime.Lifetime;
			prefabNode["InitialLifetime"] << lifetime.InitialLifetime;
		}
		if (entity.ContainsComponent<TextComponent>())
		{
			auto& text = entity.GetComponent<TextComponent>();
			ryml::NodeRef textNode = entityNode["TextComponent"];
			textNode |= ryml::MAP;
			textNode["Text"] << text.Text;
			textNode["FontHandle"] << (uint64_t)text.FontHandle;
			Util::SerializeVector4f(textNode["Color"], text.Color);
		}
		if (entity.ContainsComponent<DisabledComponent>())
		{
			auto disabledNode = entityNode["DisabledComponent"];
			disabledNode |= ryml::MAP;
			disabledNode["IsDisabled"] << true;
		}
		if (entity.ContainsComponent<PoolComponent>())
		{
			auto& pool = entity.GetComponent<PoolComponent>();
			ryml::NodeRef poolNode = entityNode["PoolComponent"];
			poolNode |= ryml::MAP;
			poolNode["PoolID"] << pool.PoolID;
		}
		if (entity.ContainsComponent<PoolConfigComponent>())
		{
			auto& poolConfig = entity.GetComponent<PoolConfigComponent>();
			ryml::NodeRef poolConfigNode = entityNode["PoolConfigComponent"];
			poolConfigNode |= ryml::MAP;
			poolConfigNode["PoolID"] << poolConfig.PoolID;
			poolConfigNode["Capacity"] << poolConfig.Capacity;
			poolConfigNode["PrefabHandle"] << (uint64_t)poolConfig.PrefabHandle;
			poolConfigNode["LoopEntities"] << poolConfig.LoopEntities;
		}
		if (entity.ContainsComponent<ParticleEmitterComponent>())
		{
			auto& emitter = entity.GetComponent<ParticleEmitterComponent>();
			ryml::NodeRef emitterNode = entityNode["ParticleEmitterComponent"];
			emitterNode |= ryml::MAP;
			emitterNode["EmissionRate"] << emitter.EmissionRate;
			Util::SerializeVector3f(emitterNode["Velocity"], emitter.Velocity);
			Util::SerializeVector3f(emitterNode["VelocityVariation"], emitter.VelocityVariation);

			emitterNode["Drag"] << emitter.Drag;

			emitterNode["AngularVelocity"] << emitter.AngularVelocity;
			emitterNode["AngularVelocityVariation"] << emitter.AngularVelocityVariation;

			Util::SerializeVector4f(emitterNode["ColorBegin"], emitter.ColorBegin);
			Util::SerializeVector4f(emitterNode["ColorEnd"], emitter.ColorEnd);

			emitterNode["ScaleBegin"] << emitter.ScaleBegin;
			emitterNode["ScaleEnd"] << emitter.ScaleEnd;
			emitterNode["ScaleVariation"] << emitter.ScaleVariation;

			emitterNode["TextureHandle"] << (uint64_t)emitter.TextureHandle;

			emitterNode["Lifetime"] << emitter.Lifetime;
			emitterNode["LifetimeVariation"] << emitter.LifetimeVariation;

			emitterNode["GravityMultiplier"] << emitter.GravityMultiplier;

			emitterNode["AlignWithVelocity"] << emitter.AlignWithVelocity;
			emitterNode["StretchFactor"] << emitter.StretchFactor;

			emitterNode["IsActive"] << emitter.IsActive;
		}
		if (entity.ContainsComponent<PostProcessVolumeComponent>())
		{
			auto& vol = entity.GetComponent<PostProcessVolumeComponent>();
			ryml::NodeRef volNode = entityNode["PostProcessVolumeComponent"];
			volNode |= ryml::MAP;
			volNode["Priority"] << vol.Priority;
			volNode["BlendRadius"] << vol.BlendRadius;

			volNode["BloomEnabled"] << vol.Settings.BloomEnabled;
			volNode["ColorGradeEnabled"] << vol.Settings.ColorGradeEnabled;
			volNode["FogEnabled"] << vol.Settings.FogEnabled;
			volNode["VignetteEnabled"] << vol.Settings.VignetteEnabled;

			ryml::NodeRef bloomNode = volNode["Bloom"];
			bloomNode |= ryml::MAP;
			bloomNode["Threshold"] << vol.Settings.Bloom.Threshold;
			bloomNode["Knee"] << vol.Settings.Bloom.Knee;
			bloomNode["Intensity"] << vol.Settings.Bloom.Intensity;
			bloomNode["BlurRadius"] << vol.Settings.Bloom.BlurRadius;

			ryml::NodeRef colorGradeNode = volNode["ColorGrade"];
			colorGradeNode |= ryml::MAP;
			colorGradeNode["Temperature"] << vol.Settings.ColorGrade.Temperature;
			colorGradeNode["Tint"] << vol.Settings.ColorGrade.Tint;
			colorGradeNode["Contrast"] << vol.Settings.ColorGrade.Contrast;
			colorGradeNode["Saturation"] << vol.Settings.ColorGrade.Saturation;
			Util::SerializeVector4f(colorGradeNode["Lift"], vol.Settings.ColorGrade.Lift);
			Util::SerializeVector4f(colorGradeNode["Gamma"], vol.Settings.ColorGrade.Gamma);
			Util::SerializeVector4f(colorGradeNode["Gain"], vol.Settings.ColorGrade.Gain);

			ryml::NodeRef fogNode = volNode["Fog"];
			fogNode |= ryml::MAP;
			Util::SerializeVector3f(fogNode["Color"], vol.Settings.Fog.Color);
			fogNode["Density"] << vol.Settings.Fog.Density;
			fogNode["Falloff"] << vol.Settings.Fog.Falloff;
			fogNode["StartDistance"] << vol.Settings.Fog.StartDistance;

			ryml::NodeRef vignetteNode = volNode["Vignette"];
			vignetteNode |= ryml::MAP;
			Util::SerializeVector3f(vignetteNode["Color"], vol.Settings.Vignette.Color);
			vignetteNode["Intensity"] << vol.Settings.Vignette.Intensity;
			vignetteNode["Size"] << vol.Settings.Vignette.Size;
			vignetteNode["Smoothness"] << vol.Settings.Vignette.Smoothness;

			ryml::NodeRef toneMapNode = volNode["ToneMap"];
			toneMapNode |= ryml::MAP;
			toneMapNode["Exposure"] << vol.Settings.ToneMap.Exposure;
		}
		if (entity.ContainsComponent<AudioSourceComponent>())
		{
			auto& audioSource = entity.GetComponent<AudioSourceComponent>();
			ryml::NodeRef audioNode = entityNode["AudioSourceComponent"];
			audioNode |= ryml::MAP;
			audioNode["AudioClipHandle"] << (uint64_t)audioSource.AudioClipHandle;
			audioNode["Volume"] << audioSource.Properties.Volume;
			audioNode["Looping"] << audioSource.Properties.Looping;
			audioNode["Spatialized"] << audioSource.Properties.Spatialized;
			audioNode["MinDistance"] << audioSource.Properties.MinDistance;
			audioNode["MaxDistance"] << audioSource.Properties.MaxDistance;
		}
		if (entity.ContainsComponent<AudioListenerComponent>())
		{
			auto& audioListener = entity.GetComponent<AudioListenerComponent>();
			ryml::NodeRef listenerNode = entityNode["AudioListenerComponent"];
			listenerNode |= ryml::MAP;
			listenerNode["IsActive"] << audioListener.IsActive;
			listenerNode["ListenerIndex"] << audioListener.ListenerIndex;
		}
		if (entity.ContainsComponent<WaypointComponent>())
		{
			auto& waypoint = entity.GetComponent<WaypointComponent>();
			ryml::NodeRef waypointNode = entityNode["WaypointComponent"];
			waypointNode |= ryml::MAP;
			waypointNode["ShowPaths"] << waypoint.ShowPaths;
		}
		if (entity.ContainsComponent<AIPathComponent>())
		{
			auto& aiPath = entity.GetComponent<AIPathComponent>();
			ryml::NodeRef pathNode = entityNode["AIPathComponent"];
			pathNode |= ryml::MAP;
			
			ryml::NodeRef waypointsNode = pathNode["Waypoints"];
			waypointsNode |= ryml::SEQ;
			for (const auto& wp : aiPath.Waypoints)
			{
				Util::SerializeVector3f(waypointsNode.append_child(), wp);
			}

			pathNode["Loop"] << aiPath.Loop;
			pathNode["Speed"] << aiPath.Speed;
			pathNode["ArrivalTolerance"] << aiPath.ArrivalTolerance;
		}
		if (entity.ContainsComponent<NavigationGridComponent>())
		{
			auto& navGrid = entity.GetComponent<NavigationGridComponent>();
			ryml::NodeRef navGridNode = entityNode["NavigationGridComponent"];
			navGridNode |= ryml::MAP;
			navGridNode["NodeSpacing"] << navGrid.NodeSpacing;
			navGridNode["Generated"] << navGrid.Generated;

			ryml::NodeRef gridRef = navGridNode["Grid"];
			gridRef |= ryml::SEQ; // 1. Change this to a Sequence (Array)

			for (const auto& column : navGrid.Grid)
			{
				for (const auto& node : column)
				{
					ryml::NodeRef nodeRef = gridRef.append_child();
					nodeRef |= ryml::MAP; // 2. Tell ryml that this array element is an Object

					Util::SerializeVector3f(nodeRef["Position"], node.WorldPosition);
					nodeRef["IsWalkable"] << node.IsWalkable;
					nodeRef["GridX"] << node.GridX;
					nodeRef["GridY"] << node.GridY;
				}
			}
		}
		if (entity.ContainsComponent<AIAgentComponent>())
		{
			auto& aiAgent = entity.GetComponent<AIAgentComponent>();
			ryml::NodeRef agentNode = entityNode["AIAgentComponent"];
			agentNode |= ryml::MAP;
			agentNode["Mode"] << (int)aiAgent.Mode;

			// Manual props
			ryml::NodeRef waypointsNode = agentNode["ManualWaypoints"];
			waypointsNode |= ryml::SEQ;
			for (const auto& wp : aiAgent.ManualWaypoints)
			{
				waypointsNode.append_child() << (uint64_t)wp;
			}
			agentNode["Loop"] << aiAgent.Loop;

			// Dynamic props
			agentNode["TargetEntity"] << (uint64_t)aiAgent.TargetEntity;
			agentNode["GridEntity"] << (uint64_t)aiAgent.GridEntity;
			agentNode["RecalculateInterval"] << aiAgent.RecalculateInterval;
		}
		if (entity.ContainsComponent<LocalAvoidanceComponent>())
		{
			auto& avoidance = entity.GetComponent<LocalAvoidanceComponent>();
			ryml::NodeRef avoidanceNode = entityNode["LocalAvoidanceComponent"];
			avoidanceNode |= ryml::MAP;
			avoidanceNode["AvoidanceRadius"] << avoidance.AvoidanceRadius;
			avoidanceNode["AvoidanceStrength"] << avoidance.AvoidanceStrength;
			avoidanceNode["AvoidanceMask"] << avoidance.AvoidanceMask;
		}
		if (entity.ContainsComponent<CanvasComponent>())
		{
			auto& canvas = entity.GetComponent<CanvasComponent>();
			ryml::NodeRef canvasNode = entityNode["CanvasComponent"];
			canvasNode |= ryml::MAP;
			canvasNode["RenderMode"] << (int)canvas.RenderMode;
			canvasNode["SortOrder"] << canvas.SortOrder;
			canvasNode["MatchWidthOrHeight"] << canvas.MatchWidthOrHeight;
			canvasNode["PlaneDistance"] << canvas.PlaneDistance;
			Util::SerializeVector2f(canvasNode["ReferenceResolution"], canvas.ReferenceResolution);
		}
		if (entity.ContainsComponent<RectTransformComponent>())
		{
			auto& rectTransform = entity.GetComponent<RectTransformComponent>();
			ryml::NodeRef rectNode = entityNode["RectTransformComponent"];
			rectNode |= ryml::MAP;
			Util::SerializeVector2f(rectNode["AnchoredPosition"], rectTransform.AnchoredPosition);
			Util::SerializeVector2f(rectNode["SizeDelta"], rectTransform.SizeDelta);
			Util::SerializeVector2f(rectNode["AnchorMin"], rectTransform.AnchorMin);
			Util::SerializeVector2f(rectNode["AnchorMax"], rectTransform.AnchorMax);
			Util::SerializeVector2f(rectNode["Pivot"], rectTransform.Pivot);
			rectNode["Rotation"] << rectTransform.Rotation;
		}
	}

	// =========================================================================
	// HELPER: DESERIALIZE SINGLE ENTITY (WITH REMAPPING SUPPORT)
	// =========================================================================
	void SceneSerializer::DeserializeEntityNode(ryml::NodeRef& entityNode, Entity deserializedEntity, const std::unordered_map<uint64_t, UUID>& uuidRemap)
	{
		// Safe remap function. If the map is empty, we are doing a standard scene load, so use original IDs.
		auto getRemappedUUID = [&](uint64_t oldID) -> UUID {
			if (oldID == (uint64_t)Constants::InvalidUUID) return Constants::InvalidUUID;
			if (uuidRemap.empty()) return UUID(oldID);
			if (uuidRemap.find(oldID) != uuidRemap.end()) return uuidRemap.at(oldID);
			return Constants::InvalidUUID; // ID was external to the prefab hierarchy
			};

		auto getRemappedScriptEntityRef = [&](uint64_t oldID) -> UUID {
			if (oldID == (uint64_t)Constants::InvalidUUID) return Constants::InvalidUUID;
			if (uuidRemap.empty()) return UUID(oldID);
			if (uuidRemap.find(oldID) != uuidRemap.end()) return uuidRemap.at(oldID);
			return UUID(oldID);
			};

		// Core components are attached automatically by AddEntity, this just updates them
		if (entityNode.has_child("RelationshipComponent"))
		{
			auto& rc = deserializedEntity.GetComponent<RelationshipComponent>();
			ryml::NodeRef rcNode = entityNode["RelationshipComponent"];

			uint64_t parentVal;
			rcNode["Parent"] >> parentVal;
			rc.ParentHandle = getRemappedUUID(parentVal);

			if (rcNode.has_child("Children"))
			{
				for (ryml::NodeRef childNode : rcNode["Children"].children())
				{
					uint64_t childVal;
					childNode >> childVal;
					UUID newChildID = getRemappedUUID(childVal);
					if (newChildID != Constants::InvalidUUID)
						rc.Children.push_back(newChildID);
				}
			}

			if (rcNode.has_child("IsAttachment"))
				rcNode["IsAttachment"] >> rc.IsAttachment;
		}

		if (entityNode.has_child("TransformComponent"))
		{
			auto& tc = deserializedEntity.GetComponent<TransformComponent>();
			ryml::NodeRef tcNode = entityNode["TransformComponent"];

			Util::DeserializeVector3f(tcNode["Position"], tc.Position);
			Util::DeserializeVector3f(tcNode["Rotation"], tc.Rotation);
			Util::DeserializeVector3f(tcNode["Scale"], tc.Scale);

			if (tcNode.has_child("WorldTransform"))
				Util::DeserializeMatrix4f(tcNode["WorldTransform"], tc.WorldTransform);
		}

		if (entityNode.has_child("SpriteComponent"))
		{
			ryml::NodeRef spriteNode = entityNode["SpriteComponent"];
			Vector4f color;
			Util::DeserializeVector4f(spriteNode["Color"], color);

			auto& sc = deserializedEntity.AttachComponent<SpriteComponent>(color);

			if (spriteNode.has_child("TextureUUID"))
			{
				uint64_t texId;
				spriteNode["TextureUUID"] >> texId;
				sc.TextureHandle = (UUID)texId;
			}

			if (spriteNode.has_child("IsBillboard"))
				spriteNode["IsBillboard"] >> sc.IsBillboard;

			if (spriteNode.has_child("LockYAxis"))
				spriteNode["LockYAxis"] >> sc.LockYAxis;
		}

		if (entityNode.has_child("RigidBodyComponent"))
		{
			ryml::NodeRef rbNode = entityNode["RigidBodyComponent"];

			// Deserialize into a temporary first so the OnComponentAttached hook
			// (which calls CreateRigidBody) sees the correct Type, Mass, GravityEnabled, etc
			RigidBodyComponent rbc;
			int typeVal;
			rbNode["Type"] >> typeVal;
			rbc.Type = static_cast<RigidBodyComponent::BodyType>(typeVal);
			rbNode["Mass"] >> rbc.Mass;
			rbNode["GravityEnabled"] >> rbc.GravityEnabled;

			deserializedEntity.AttachComponent<RigidBodyComponent>(rbc);
		}

		if (entityNode.has_child("BoxColliderComponent"))
		{
			ryml::NodeRef colliderNode = entityNode["BoxColliderComponent"];
			auto& bcc = deserializedEntity.AttachComponent<BoxColliderComponent>();
			Util::DeserializeVector3f(colliderNode["Size"], bcc.Size);
			if (colliderNode.has_child("OffsetPosition"))
				Util::DeserializeVector3f(colliderNode["OffsetPosition"], bcc.Offset.Position);
			else if (colliderNode.has_child("Offset"))
				Util::DeserializeVector3f(colliderNode["Offset"], bcc.Offset.Position);
			if (colliderNode.has_child("OffsetRotation"))
				Util::DeserializeVector3f(colliderNode["OffsetRotation"], bcc.Offset.Rotation);
			colliderNode["Category"] >> bcc.Category;
			colliderNode["CollisionMask"] >> bcc.CollisionMask;
			colliderNode["IsTrigger"] >> bcc.IsTrigger;
			if (colliderNode.has_child("PreviewCollider"))
				colliderNode["PreviewCollider"] >> bcc.PreviewCollider;
			uint64_t bccPhysMatId;
			colliderNode["PhysicsMaterialUUID"] >> bccPhysMatId;
			bcc.PhysicsMaterialHandle = (UUID)bccPhysMatId;
		}

		if (entityNode.has_child("SphereColliderComponent"))
		{
			ryml::NodeRef colliderNode = entityNode["SphereColliderComponent"];
			auto& scc = deserializedEntity.AttachComponent<SphereColliderComponent>();
			colliderNode["Radius"] >> scc.Radius;
			colliderNode["IsTrigger"] >> scc.IsTrigger;
			if (colliderNode.has_child("PreviewCollider"))
				colliderNode["PreviewCollider"] >> scc.PreviewCollider;
			if (colliderNode.has_child("OffsetPosition"))
				Util::DeserializeVector3f(colliderNode["OffsetPosition"], scc.Offset.Position);
			else if (colliderNode.has_child("Offset"))
				Util::DeserializeVector3f(colliderNode["Offset"], scc.Offset.Position);
			if (colliderNode.has_child("OffsetRotation"))
				Util::DeserializeVector3f(colliderNode["OffsetRotation"], scc.Offset.Rotation);
			colliderNode["Category"] >> scc.Category;
			colliderNode["CollisionMask"] >> scc.CollisionMask;
			uint64_t sccPhysMatId;
			colliderNode["PhysicsMaterialUUID"] >> sccPhysMatId;
			scc.PhysicsMaterialHandle = (UUID)sccPhysMatId;
		}

		if (entityNode.has_child("CapsuleColliderComponent"))
		{
			ryml::NodeRef colliderNode = entityNode["CapsuleColliderComponent"];
			auto& ccc = deserializedEntity.AttachComponent<CapsuleColliderComponent>();
			colliderNode["Radius"] >> ccc.Radius;
			colliderNode["Height"] >> ccc.Height;
			colliderNode["IsTrigger"] >> ccc.IsTrigger;
			if (colliderNode.has_child("PreviewCollider"))
				colliderNode["PreviewCollider"] >> ccc.PreviewCollider;
			if (colliderNode.has_child("OffsetPosition"))
				Util::DeserializeVector3f(colliderNode["OffsetPosition"], ccc.Offset.Position);
			else if (colliderNode.has_child("Offset"))
				Util::DeserializeVector3f(colliderNode["Offset"], ccc.Offset.Position);
			if (colliderNode.has_child("OffsetRotation"))
				Util::DeserializeVector3f(colliderNode["OffsetRotation"], ccc.Offset.Rotation);
			colliderNode["Category"] >> ccc.Category;
			colliderNode["CollisionMask"] >> ccc.CollisionMask;
			uint64_t cccPhysMatId;
			colliderNode["PhysicsMaterialUUID"] >> cccPhysMatId;
			ccc.PhysicsMaterialHandle = (UUID)cccPhysMatId;
		}

		if (entityNode.has_child("ConvexMeshColliderComponent"))
		{
			ryml::NodeRef colliderNode = entityNode["ConvexMeshColliderComponent"];
			uint64_t meshId;
			colliderNode["MeshUUID"] >> meshId;
			UUID meshUUID = (UUID)meshId;

			auto& ccc = deserializedEntity.AttachComponent<ConvexMeshColliderComponent>();
			ccc.MeshHandle = meshUUID;
			colliderNode["IsTrigger"] >> ccc.IsTrigger;
			if (colliderNode.has_child("PreviewCollider"))
				colliderNode["PreviewCollider"] >> ccc.PreviewCollider;
			if (colliderNode.has_child("OffsetPosition"))
				Util::DeserializeVector3f(colliderNode["OffsetPosition"], ccc.Offset.Position);
			if (colliderNode.has_child("OffsetRotation"))
				Util::DeserializeVector3f(colliderNode["OffsetRotation"], ccc.Offset.Rotation);
			colliderNode["Category"] >> ccc.Category;
			colliderNode["CollisionMask"] >> ccc.CollisionMask;
			uint64_t convexPhysMatId;
			colliderNode["PhysicsMaterialUUID"] >> convexPhysMatId;
			ccc.PhysicsMaterialHandle = (UUID)convexPhysMatId;
		}

		if (entityNode.has_child("ConcaveMeshColliderComponent"))
		{
			ryml::NodeRef colliderNode = entityNode["ConcaveMeshColliderComponent"];
			uint64_t meshId;
			colliderNode["MeshUUID"] >> meshId;
			UUID meshUUID = (UUID)meshId;

			auto& cmcc = deserializedEntity.AttachComponent<ConcaveMeshColliderComponent>();
			cmcc.MeshHandle = meshUUID;
			colliderNode["IsTrigger"] >> cmcc.IsTrigger;
			if (colliderNode.has_child("PreviewCollider"))
				colliderNode["PreviewCollider"] >> cmcc.PreviewCollider;
			if (colliderNode.has_child("OffsetPosition"))
				Util::DeserializeVector3f(colliderNode["OffsetPosition"], cmcc.Offset.Position);
			if (colliderNode.has_child("OffsetRotation"))
				Util::DeserializeVector3f(colliderNode["OffsetRotation"], cmcc.Offset.Rotation);
			colliderNode["Category"] >> cmcc.Category;
			colliderNode["CollisionMask"] >> cmcc.CollisionMask;
			uint64_t concavePhysMatId;
			colliderNode["PhysicsMaterialUUID"] >> concavePhysMatId;
			cmcc.PhysicsMaterialHandle = (UUID)concavePhysMatId;
		}

		if (entityNode.has_child("StaticMeshComponent"))
		{
			ryml::NodeRef meshNode = entityNode["StaticMeshComponent"];
			uint64_t meshId;
			meshNode["MeshUUID"] >> meshId;

			auto& mc = deserializedEntity.AttachComponent<StaticMeshComponent>();
			mc.MeshHandle = (UUID)meshId;

			meshNode["Layer"] >> mc.Layer;
		}

		if (entityNode.has_child("SkinnedMeshComponent"))
		{
			ryml::NodeRef meshNode = entityNode["SkinnedMeshComponent"];
			uint64_t meshId;
			meshNode["MeshUUID"] >> meshId;

			uint64_t animatorId;
			meshNode["RootAnimator"] >> animatorId;

			auto& mc = deserializedEntity.AttachComponent<SkinnedMeshComponent>();
			mc.MeshHandle = (UUID)meshId;
			mc.AnimatorEntityHandle = getRemappedUUID(animatorId);

			meshNode["Layer"] >> mc.Layer;
		}

		if (entityNode.has_child("MaterialComponent"))
		{
			ryml::NodeRef materialNode = entityNode["MaterialComponent"];

			uint64_t materialId;
			materialNode["MaterialUUID"] >> materialId;

			auto& matComp = deserializedEntity.AttachComponent<MaterialComponent>();
			matComp.MaterialHandle = (UUID)materialId;
			if (matComp.MaterialHandle == Constants::InvalidUUID)
			{
				EB_CORE_ERROR("Deserializer failed to load Material. Assigning Fallback.");
				matComp.MaterialHandle = Constants::Assets::DefaultMatUUID;
			}
		}

		if (entityNode.has_child("CameraComponent"))
		{
			ryml::NodeRef cameraNode = entityNode["CameraComponent"];
			auto& cc = deserializedEntity.AttachComponent<CameraComponent>();

			if (cameraNode.has_child("Projection")) {
				Matrix4f proj;
				Util::DeserializeMatrix4f(cameraNode["Projection"], proj);
				cc.Camera.SetProjectionMatrix(proj);
			}

			int typeVal;
			cameraNode["Type"] >> typeVal;
			cc.Camera.SetProjectionType((Camera::ProjectionType)typeVal);

			bool isActive;
			cameraNode["IsActive"] >> isActive;
			cc.IsActive = isActive;

			cameraNode["RenderMask"] >> cc.RenderMask;
			cameraNode["VolumeMask"] >> cc.VolumeMask;

			if (cameraNode.has_child("OrthographicProperties"))
			{
				ryml::NodeRef orthoNode = cameraNode["OrthographicProperties"];
				orthoNode["Size"] >> cc.Camera.GetOrthographicProps().Size;
				orthoNode["NearClip"] >> cc.Camera.GetOrthographicProps().NearClip;
				orthoNode["FarClip"] >> cc.Camera.GetOrthographicProps().FarClip;
			}

			if (cameraNode.has_child("PerspectiveProperties"))
			{
				ryml::NodeRef perspNode = cameraNode["PerspectiveProperties"];
				perspNode["FOV"] >> cc.Camera.GetPerspectiveProps().FieldOfView;
				perspNode["NearClip"] >> cc.Camera.GetPerspectiveProps().NearClip;
				perspNode["FarClip"] >> cc.Camera.GetPerspectiveProps().FarClip;
			}
		}

		if (entityNode.has_child("DirectionalLightComponent"))
		{
			ryml::NodeRef lightNode = entityNode["DirectionalLightComponent"];
			auto& dlc = deserializedEntity.AttachComponent<DirectionalLightComponent>();
			if (lightNode.has_child("Active"))
				lightNode["Active"] >> dlc.Active;
			Util::DeserializeVector3f(lightNode["Color"], dlc.Color);
			lightNode["Intensity"] >> dlc.Intensity;
		}

		if (entityNode.has_child("SpotLightComponent"))
		{
			ryml::NodeRef lightNode = entityNode["SpotLightComponent"];

			auto& slc = deserializedEntity.AttachComponent<SpotLightComponent>();
			if (lightNode.has_child("Active"))
				lightNode["Active"] >> slc.Active;

			Util::DeserializeVector3f(lightNode["Color"], slc.Color);
			lightNode["Intensity"] >> slc.Intensity;

			lightNode["CutOffAngle"] >> slc.CutOffAngle;
			slc.CutOff = std::cos(slc.CutOffAngle);

			lightNode["OuterCutOffAngle"] >> slc.OuterCutOffAngle;
			slc.OuterCutOff = std::cos(slc.OuterCutOffAngle);
		}

		if (entityNode.has_child("PointLightComponent"))
		{
			ryml::NodeRef lightNode = entityNode["PointLightComponent"];
			auto& plc = deserializedEntity.AttachComponent<PointLightComponent>();
			if (lightNode.has_child("Active"))
				lightNode["Active"] >> plc.Active;
			Util::DeserializeVector3f(lightNode["Color"], plc.Color);
			lightNode["Intensity"] >> plc.Intensity;
			lightNode["Radius"] >> plc.Radius;
		}

		if (entityNode.has_child("ScriptComponent"))
		{
			auto scriptNode = entityNode["ScriptComponent"];
			uint64_t uuidVal;
			scriptNode["ScriptUUID"] >> uuidVal;

			std::unordered_map<std::string, ScriptProperty> scriptUserOverrides;
			if (scriptNode.has_child("UserPropertyOverrides"))
			{
				ryml::NodeRef overridesNode = scriptNode["UserPropertyOverrides"];
				for (ryml::NodeRef propNode : overridesNode.children())
				{
					std::string propName;
					propNode["Name"] >> propName;

					int typeInt;
					propNode["Type"] >> typeInt;
					ScriptPropertyType propType = (ScriptPropertyType)typeInt;

					ScriptPropertyValue propValue;

					switch (propType)
					{
					case ScriptPropertyType::Float:
					{
						float val;
						propNode["Value"] >> val;
						propValue = val;
						break;
					}
					case ScriptPropertyType::Int:
					case ScriptPropertyType::Enum:
					{
						int val;
						propNode["Value"] >> val;
						propValue = val;
						break;
					}
					case ScriptPropertyType::Bool:
					{
						bool val;
						propNode["Value"] >> val;
						propValue = val;
						break;
					}
					case ScriptPropertyType::String:
					{
						std::string val;
						propNode["Value"] >> val;
						propValue = val;
						break;
					}
					case ScriptPropertyType::Vector3f:
					{
						Vector3f val;
						Util::DeserializeVector3f(propNode["Value"], val);
						propValue = val;
						break;
					}
					case ScriptPropertyType::EntityRef:
					case ScriptPropertyType::AssetRef:
					{
						uint64_t val = Constants::InvalidUUID;
						propNode["Value"] >> val;
						propValue = propType == ScriptPropertyType::EntityRef ? getRemappedScriptEntityRef(val) : UUID(val);
						break;
					}
					default:
						break;
					}

					ScriptReferenceKind referenceKind = ScriptReferenceKind::None;
					if (propNode.has_child("ReferenceKind"))
					{
						std::string referenceKindName;
						propNode["ReferenceKind"] >> referenceKindName;
						referenceKind = ScriptReferenceKindFromString(referenceKindName);
					}

					scriptUserOverrides[propName] = { propName, propValue, propType, referenceKind };
				}
			}

			auto& sc = deserializedEntity.AttachComponent<ScriptComponent>((UUID)uuidVal);
			sc.UserPropertyOverrides = scriptUserOverrides;
		}

		//if (entityNode.has_child("OutlineComponent"))
		//{
		//	ryml::NodeRef outlineNode = entityNode["OutlineComponent"];
		//	OutlineComponent oc;
		//	Util::DeserializeVector3f(outlineNode["Color"], oc.Color);
		//	outlineNode["Thickness"] >> oc.Thickness;
		//	deserializedEntity.AttachComponent<OutlineComponent>(oc);
		//}

		if (entityNode.has_child("AnimatorComponent"))
		{
			ryml::NodeRef animatorNode = entityNode["AnimatorComponent"];
			auto& ac = deserializedEntity.AttachComponent<AnimatorComponent>();

			uint64_t skelHandle = Constants::InvalidUUID;
			if (animatorNode.has_child("SkeletonHandle"))
				animatorNode["SkeletonHandle"] >> skelHandle;

			uint64_t animHandle = Constants::InvalidUUID;
			if (animatorNode.has_child("CurrentAnimationHandle"))
				animatorNode["CurrentAnimationHandle"] >> animHandle;

			ac.SkeletonHandle = (UUID)skelHandle;
			ac.CurrentAnimationHandle = (UUID)animHandle;
		}

		if (entityNode.has_child("BoneSocketComponent"))
		{
			ryml::NodeRef socketNode = entityNode["BoneSocketComponent"];
			auto& socket = deserializedEntity.AttachComponent<BoneSocketComponent>();

			uint64_t targetEntity = Constants::InvalidUUID;
			if (socketNode.has_child("TargetEntity"))
				socketNode["TargetEntity"] >> targetEntity;

			socket.TargetEntityHandle = getRemappedUUID(targetEntity);
			if (socketNode.has_child("BoneName"))
				socketNode["BoneName"] >> socket.BoneName;
			if (socketNode.has_child("Position"))
				Util::DeserializeVector3f(socketNode["Position"], socket.Position);
			if (socketNode.has_child("Rotation"))
				Util::DeserializeVector3f(socketNode["Rotation"], socket.Rotation);
			if (socketNode.has_child("Scale"))
				Util::DeserializeVector3f(socketNode["Scale"], socket.Scale);
		}

		if (entityNode.has_child("PrefabComponent"))
		{
			ryml::NodeRef prefabNode = entityNode["PrefabComponent"];
			uint64_t prefabId;
			prefabNode["PrefabUUID"] >> prefabId;
			auto& pc = deserializedEntity.AttachComponent<PrefabComponent>();
			pc.PrefabHandle = (UUID)prefabId;
		}

		if (entityNode.has_child("CharacterControllerComponent"))
		{
			ryml::NodeRef controllerNode = entityNode["CharacterControllerComponent"];
			auto& ccc = deserializedEntity.AttachComponent<CharacterControllerComponent>();
			controllerNode["WalkSpeed"] >> ccc.WalkSpeed;
			controllerNode["JumpForce"] >> ccc.JumpForce;
			controllerNode["GravityMultiplier"] >> ccc.GravityMultiplier;
			controllerNode["MaxSlopeAngle"] >> ccc.MaxSlopeAngle;
			controllerNode["MaxStepHeight"] >> ccc.MaxStepHeight;
		}

		if (entityNode.has_child("LifetimeComponent"))
		{
			ryml::NodeRef lifetimeNode = entityNode["LifetimeComponent"];
			auto& ltc = deserializedEntity.AttachComponent<LifetimeComponent>();
			lifetimeNode["Lifetime"] >> ltc.Lifetime;

			// Backward-compatible: older assets only store Lifetime.
			if (lifetimeNode.has_child("InitialLifetime"))
				lifetimeNode["InitialLifetime"] >> ltc.InitialLifetime;
			else
				ltc.InitialLifetime = ltc.Lifetime;
		}
		
		if (entityNode.has_child("TextComponent"))
		{
			ryml::NodeRef textNode = entityNode["TextComponent"];
			auto& tc = deserializedEntity.AttachComponent<TextComponent>();
			textNode["Text"] >> tc.Text;
			uint64_t fontId;
			textNode["FontHandle"] >> fontId;
			tc.FontHandle = (UUID)fontId;
			Util::DeserializeVector4f(textNode["Color"], tc.Color);
		}

		if (entityNode.has_child("DisabledComponent"))
		{
			// No props just attach the component to mark the entity as disabled
			deserializedEntity.AttachComponent<DisabledComponent>();
		}

		if (entityNode.has_child("PoolComponent"))
		{
			ryml::NodeRef poolNode = entityNode["PoolComponent"];

			auto& pc = deserializedEntity.AttachComponent<PoolComponent>();
			poolNode["PoolID"] >> pc.PoolID;
		}

		if (entityNode.has_child("PoolConfigComponent"))
		{
			ryml::NodeRef poolNode = entityNode["PoolConfigComponent"];

			auto& pcc = deserializedEntity.AttachComponent<PoolConfigComponent>();
			poolNode["PoolID"] >> pcc.PoolID;
			poolNode["Capacity"] >> pcc.Capacity;

			uint64_t prefabId;
			poolNode["PrefabHandle"] >> prefabId;
			pcc.PrefabHandle = (UUID)prefabId;
			poolNode["LoopEntities"] >> pcc.LoopEntities;
		}

		if (entityNode.has_child("PostProcessVolumeComponent"))
		{
			ryml::NodeRef volNode = entityNode["PostProcessVolumeComponent"];
			auto& vol = deserializedEntity.AttachComponent<PostProcessVolumeComponent>();
			volNode["Priority"] >> vol.Priority;
			volNode["BlendRadius"] >> vol.BlendRadius;

			volNode["BloomEnabled"] >> vol.Settings.BloomEnabled;
			volNode["ColorGradeEnabled"] >> vol.Settings.ColorGradeEnabled;
			volNode["FogEnabled"] >> vol.Settings.FogEnabled;
			volNode["VignetteEnabled"] >> vol.Settings.VignetteEnabled;

			if (volNode.has_child("Bloom"))
			{
				ryml::NodeRef bloomNode = volNode["Bloom"];
				bloomNode["Threshold"] >> vol.Settings.Bloom.Threshold;
				bloomNode["Knee"] >> vol.Settings.Bloom.Knee;
				bloomNode["Intensity"] >> vol.Settings.Bloom.Intensity;
				bloomNode["BlurRadius"] >> vol.Settings.Bloom.BlurRadius;
			}

			if (volNode.has_child("ColorGrade"))
			{
				ryml::NodeRef colorGradeNode = volNode["ColorGrade"];
				colorGradeNode["Temperature"] >> vol.Settings.ColorGrade.Temperature;
				colorGradeNode["Tint"] >> vol.Settings.ColorGrade.Tint;
				colorGradeNode["Contrast"] >> vol.Settings.ColorGrade.Contrast;
				colorGradeNode["Saturation"] >> vol.Settings.ColorGrade.Saturation;
				Util::DeserializeVector4f(colorGradeNode["Lift"], vol.Settings.ColorGrade.Lift);
				Util::DeserializeVector4f(colorGradeNode["Gamma"], vol.Settings.ColorGrade.Gamma);
				Util::DeserializeVector4f(colorGradeNode["Gain"], vol.Settings.ColorGrade.Gain);
			}

			if (volNode.has_child("Fog"))
			{
				ryml::NodeRef fogNode = volNode["Fog"];
				Util::DeserializeVector3f(fogNode["Color"], vol.Settings.Fog.Color);
				fogNode["Density"] >> vol.Settings.Fog.Density;
				fogNode["Falloff"] >> vol.Settings.Fog.Falloff;
				fogNode["StartDistance"] >> vol.Settings.Fog.StartDistance;
			}

			if (volNode.has_child("Vignette"))
			{
				ryml::NodeRef vignetteNode = volNode["Vignette"];
				Util::DeserializeVector3f(vignetteNode["Color"], vol.Settings.Vignette.Color);
				vignetteNode["Intensity"] >> vol.Settings.Vignette.Intensity;
				vignetteNode["Size"] >> vol.Settings.Vignette.Size;
				vignetteNode["Smoothness"] >> vol.Settings.Vignette.Smoothness;
			}

			if (volNode.has_child("ToneMap"))
			{
				ryml::NodeRef toneMapNode = volNode["ToneMap"];
				toneMapNode["Exposure"] >> vol.Settings.ToneMap.Exposure;
			}
		}

		if (entityNode.has_child("ParticleEmitterComponent"))
		{
			ryml::NodeRef emitterNode = entityNode["ParticleEmitterComponent"];
			auto& pec = deserializedEntity.AttachComponent<ParticleEmitterComponent>();
			emitterNode["EmissionRate"] >> pec.EmissionRate;
			Util::DeserializeVector3f(emitterNode["Velocity"], pec.Velocity);
			Util::DeserializeVector3f(emitterNode["VelocityVariation"], pec.VelocityVariation);
			emitterNode["Drag"] >> pec.Drag;
			emitterNode["AngularVelocity"] >> pec.AngularVelocity;
			emitterNode["AngularVelocityVariation"] >> pec.AngularVelocityVariation;
			Util::DeserializeVector4f(emitterNode["ColorBegin"], pec.ColorBegin);
			Util::DeserializeVector4f(emitterNode["ColorEnd"], pec.ColorEnd);
			emitterNode["ScaleBegin"] >> pec.ScaleBegin;
			emitterNode["ScaleEnd"] >> pec.ScaleEnd;
			emitterNode["ScaleVariation"] >> pec.ScaleVariation;
			uint64_t texId;
			emitterNode["TextureHandle"] >> texId;
			pec.TextureHandle = (UUID)texId;
			emitterNode["Lifetime"] >> pec.Lifetime;
			emitterNode["LifetimeVariation"] >> pec.LifetimeVariation;
			emitterNode["GravityMultiplier"] >> pec.GravityMultiplier;
			emitterNode["AlignWithVelocity"] >> pec.AlignWithVelocity;
			emitterNode["StretchFactor"] >> pec.StretchFactor;
			emitterNode["IsActive"] >> pec.IsActive;
		}

		if (entityNode.has_child("AudioSourceComponent"))
		{
			ryml::NodeRef audioNode = entityNode["AudioSourceComponent"];
			auto& audioSource = deserializedEntity.AttachComponent<AudioSourceComponent>();

			uint64_t audioClipId;
			audioNode["AudioClipHandle"] >> audioClipId;
			audioSource.AudioClipHandle = (UUID)audioClipId;
			audioNode["Volume"] >> audioSource.Properties.Volume;
			audioNode["Looping"] >> audioSource.Properties.Looping;
			audioNode["Spatialized"] >> audioSource.Properties.Spatialized;
			audioNode["MinDistance"] >> audioSource.Properties.MinDistance;
			audioNode["MaxDistance"] >> audioSource.Properties.MaxDistance;
		}

		if (entityNode.has_child("AudioListenerComponent"))
		{
			ryml::NodeRef listenerNode = entityNode["AudioListenerComponent"];
			auto& alc = deserializedEntity.AttachComponent<AudioListenerComponent>();
			listenerNode["IsActive"] >> alc.IsActive;
			listenerNode["ListenerIndex"] >> alc.ListenerIndex;
		}

		if (entityNode.has_child("WaypointComponent"))
		{
			ryml::NodeRef waypointNode = entityNode["WaypointComponent"];
			auto& wpc = deserializedEntity.AttachComponent<WaypointComponent>();
			waypointNode["ShowPaths"] >> wpc.ShowPaths;
		}

		if (entityNode.has_child("AIPathComponent"))
		{
			ryml::NodeRef pathNode = entityNode["AIPathComponent"];
			auto& aiPath = deserializedEntity.AttachComponent<AIPathComponent>();
			if (pathNode.has_child("Waypoints"))
			{
				for (ryml::NodeRef wpNode : pathNode["Waypoints"].children())
				{
					Vector3f wp;
					Util::DeserializeVector3f(wpNode, wp);
					aiPath.Waypoints.push_back(wp);
				}
			}
			pathNode["Loop"] >> aiPath.Loop;
			pathNode["Speed"] >> aiPath.Speed;
			pathNode["ArrivalTolerance"] >> aiPath.ArrivalTolerance;
		}

		if (entityNode.has_child("NavigationGridComponent"))
		{
			ryml::NodeRef navGridNode = entityNode["NavigationGridComponent"];
			auto& navGrid = deserializedEntity.AttachComponent<NavigationGridComponent>();
			navGridNode["NodeSpacing"] >> navGrid.NodeSpacing;
			navGridNode["Generated"] >> navGrid.Generated;
			if (navGridNode.has_child("Grid"))
			{
				std::vector<std::vector<NavNode>> grid;
				for (ryml::NodeRef nodeRef : navGridNode["Grid"].children())
				{
					Vector3f pos;
					Util::DeserializeVector3f(nodeRef["Position"], pos);
					bool isWalkable;
					nodeRef["IsWalkable"] >> isWalkable;
					int gridX, gridY;
					nodeRef["GridX"] >> gridX;
					nodeRef["GridY"] >> gridY;

					if (grid.size() <= gridX)
						grid.resize(gridX + 1);
					if (grid[gridX].size() <= gridY)
						grid[gridX].resize(gridY + 1);

					grid[gridX][gridY] = { pos, gridX, gridY, isWalkable };
				}

				navGrid.Grid = grid;
			}
		}

		if (entityNode.has_child("AIAgentComponent"))
		{
			ryml::NodeRef aiAgentNode = entityNode["AIAgentComponent"];
			auto& aiAgent = deserializedEntity.AttachComponent<AIAgentComponent>();

			int modeVal;
			aiAgentNode["Mode"] >> modeVal;
			aiAgent.Mode = (AIAgentComponent::PathMode)modeVal;

			if (aiAgentNode.has_child("ManualWaypoints"))
			{
				for (ryml::NodeRef wpNode : aiAgentNode["ManualWaypoints"].children())
				{
					uint64_t wpEntityVal;
					wpNode >> wpEntityVal;
					UUID wpEntityID = (UUID)wpEntityVal;
					if (wpEntityID != Constants::InvalidUUID)
						aiAgent.ManualWaypoints.push_back(wpEntityID);
				}
			}

			aiAgentNode["Loop"] >> aiAgent.Loop;

			uint64_t targetEntityVal, gridEntityVal;
			aiAgentNode["TargetEntity"] >> targetEntityVal;
			aiAgent.TargetEntity = (UUID)targetEntityVal;

			aiAgentNode["GridEntity"] >> gridEntityVal;
			aiAgent.GridEntity = (UUID)gridEntityVal;

			aiAgentNode["RecalculateInterval"] >> aiAgent.RecalculateInterval;
		}

		if (entityNode.has_child("LocalAvoidanceComponent"))
		{
			ryml::NodeRef avoidanceNode = entityNode["LocalAvoidanceComponent"];
			auto& avoidance = deserializedEntity.AttachComponent<LocalAvoidanceComponent>();
			avoidanceNode["AvoidanceRadius"] >> avoidance.AvoidanceRadius;
			avoidanceNode["AvoidanceStrength"] >> avoidance.AvoidanceStrength;
			avoidanceNode["AvoidanceMask"] >> avoidance.AvoidanceMask;
		}

		if (entityNode.has_child("CanvasComponent"))
		{
			ryml::NodeRef canvasNode = entityNode["CanvasComponent"];
			auto& canvas = deserializedEntity.AttachComponent<CanvasComponent>();
			int renderModeInt = 0;
			canvasNode["RenderMode"] >> renderModeInt;
			canvas.RenderMode = static_cast<CanvasRenderMode>(renderModeInt);
			canvasNode["SortOrder"] >> canvas.SortOrder;
			canvasNode["PlaneDistance"] >> canvas.PlaneDistance;
			canvasNode["MatchWidthOrHeight"] >> canvas.MatchWidthOrHeight;
			Util::DeserializeVector2f(canvasNode["ReferenceResolution"], canvas.ReferenceResolution);
		}

		if (entityNode.has_child("RectTransformComponent"))
		{
			ryml::NodeRef rectNode = entityNode["RectTransformComponent"];
			auto& rect = deserializedEntity.AttachComponent<RectTransformComponent>();
			Util::DeserializeVector2f(rectNode["AnchorMin"], rect.AnchorMin);
			Util::DeserializeVector2f(rectNode["AnchorMax"], rect.AnchorMax);
			Util::DeserializeVector2f(rectNode["Pivot"], rect.Pivot);
			Util::DeserializeVector2f(rectNode["SizeDelta"], rect.SizeDelta);
			Util::DeserializeVector2f(rectNode["AnchoredPosition"], rect.AnchoredPosition);
			rectNode["Rotation"] >> rect.Rotation;
		}

		DeserializeComponentOrder(entityNode, deserializedEntity);
	}


	// =========================================================================
	// SCENE SERIALIZATION (MAIN)
	// =========================================================================
	bool SceneSerializer::Serialize(const std::string& filepath)
	{
		ryml::Tree tree;
		ryml::NodeRef root = tree.rootref();
		root |= ryml::MAP;

		root["Scene"] << m_Scene->GetName();
		ryml::NodeRef entitiesNode = root["Entities"];
		entitiesNode |= ryml::SEQ;

		auto entities = m_Scene->GetAllEntities();
		for (Entity entity : entities)
		{
			if (entity == Constants::Entities::InvalidEntityID)
				continue;

			ryml::NodeRef entityNode = entitiesNode.append_child();
			SerializeEntityNode(entityNode, entity);
		}

		// Environment settings
		auto renderSystem = Application::Instance().GetSystem<RenderSystem>();
		auto skybox = renderSystem->GetSkybox();
		auto bloomPass = StaticPointerCast<BloomPass>(renderSystem->GetPostProcessPass("BloomPass"));
		auto fxaaPass = StaticPointerCast<FXAAPass>(renderSystem->GetPostProcessPass("FXAAPass"));
		auto colorGradingPass = StaticPointerCast<ColorGradePass>(renderSystem->GetPostProcessPass("ColorGradePass"));
		auto toneMapPass = StaticPointerCast<ToneMapPass>(renderSystem->GetPostProcessPass("ToneMapPass"));
		auto fogPass = StaticPointerCast<FogPass>(renderSystem->GetPostProcessPass("FogPass"));
		auto vignettePass = StaticPointerCast<VignettePass>(renderSystem->GetPostProcessPass("VignettePass"));

		ryml::NodeRef envNode = root["Environment"];
		envNode |= ryml::MAP;

		ryml::NodeRef skyboxNode = envNode["Skybox"];
		skyboxNode |= ryml::MAP;
		skyboxNode["Enabled"] << skybox->Enabled();
		skyboxNode["Intensity"] << skybox->GetIntensity();
		skyboxNode["TextureUUID"] << (uint64_t)skybox->GetSkyboxTextureHandle();

		ryml::NodeRef bloomNode = envNode["Bloom"];
		bloomNode |= ryml::MAP;
		bloomNode["Enabled"] << bloomPass->Enabled;
		bloomNode["Threshold"] << bloomPass->Settings.Threshold;
		bloomNode["Knee"] << bloomPass->Settings.Knee;
		bloomNode["Intensity"] << bloomPass->Settings.Intensity;
		bloomNode["BlurRadius"] << bloomPass->Settings.BlurRadius;

		ryml::NodeRef fogNode = envNode["Fog"];
		fogNode |= ryml::MAP;
		fogNode["Enabled"] << fogPass->Enabled;
		Util::SerializeVector3f(fogNode["Color"], fogPass->Settings.Color);
		fogNode["Density"] << fogPass->Settings.Density;
		fogNode["StartDistance"] << fogPass->Settings.StartDistance;
		fogNode["Falloff"] << fogPass->Settings.Falloff;

		ryml::NodeRef fxaaNode = envNode["FXAA"];
		fxaaNode |= ryml::MAP;
		fxaaNode["Enabled"] << fxaaPass->Enabled;
		fxaaNode["SubpixelQuality"] << fxaaPass->SubpixelQuality;
		fxaaNode["EdgeThresholdMin"] << fxaaPass->EdgeThresholdMin;
		fxaaNode["EdgeThresholdMax"] << fxaaPass->EdgeThresholdMax;

		ryml::NodeRef vignetteNode = envNode["Vignette"];
		vignetteNode |= ryml::MAP;
		Util::SerializeVector3f(vignetteNode["Color"], vignettePass->Settings.Color);
		vignetteNode["Intensity"] << vignettePass->Settings.Intensity;
		vignetteNode["Smoothness"] << vignettePass->Settings.Smoothness;
		vignetteNode["Size"] << vignettePass->Settings.Size;

		ryml::NodeRef colorGradeNode = envNode["ColorGrading"];
		colorGradeNode |= ryml::MAP;
		colorGradeNode["Enabled"] << colorGradingPass->Enabled;
		colorGradeNode["Exposure"] << toneMapPass->Settings.Exposure;
		colorGradeNode["Temperature"] << colorGradingPass->Settings.Temperature;
		colorGradeNode["Tint"] << colorGradingPass->Settings.Tint;
		colorGradeNode["Contrast"] << colorGradingPass->Settings.Contrast;
		colorGradeNode["Saturation"] << colorGradingPass->Settings.Saturation;
		Util::SerializeVector4f(colorGradeNode["Lift"], colorGradingPass->Settings.Lift);
		Util::SerializeVector4f(colorGradeNode["Gamma"], colorGradingPass->Settings.Gamma);
		Util::SerializeVector4f(colorGradeNode["Gain"], colorGradingPass->Settings.Gain);

		std::ofstream fout(filepath);
		fout << tree;
		fout.close();

		return true;
	}

	bool SceneSerializer::Deserialize(const std::string& filepath)
	{
		std::ifstream stream(filepath);
		if (!stream.is_open())
		{
			EB_CORE_ERROR("Failed to open scene file: {0}", filepath);
			return false;
		}

		std::stringstream strStream;
		strStream << stream.rdbuf();
		std::string yamlData = strStream.str();

		ryml::Tree tree = ryml::parse_in_arena(ryml::to_csubstr(yamlData));
		ryml::NodeRef root = tree.rootref();

		if (!root.has_child("Scene"))
			return false;

		std::string sceneName;
		root["Scene"] >> sceneName;
		EB_CORE_TRACE("Deserializing Scene: {0}", sceneName);
		m_Scene->Clear();

		if (root.has_child("Entities"))
		{
			ryml::NodeRef entitiesNode = root["Entities"];

			// Empty map because we are loading absolute scene IDs
			std::unordered_map<uint64_t, UUID> emptyRemap;

			for (ryml::NodeRef entityNode : entitiesNode.children())
			{
				uint64_t uuidVal;
				entityNode["Entity"] >> uuidVal;
				UUID uuid = UUID(uuidVal);

				std::string name = "Entity";
				if (entityNode.has_child("TagComponent"))
					entityNode["TagComponent"]["Tag"] >> name;

				bool isUIEntity = entityNode.has_child("RectTransformComponent");
				Entity deserializedEntity = m_Scene->AddEntity(uuid, name);
				DeserializeEntityNode(entityNode, deserializedEntity, emptyRemap);
			}
		}

		if (root.has_child("Environment"))
		{
			auto renderSystem = Application::Instance().GetSystem<RenderSystem>();

			ryml::NodeRef envNode = root["Environment"];
			if (envNode.has_child("Skybox"))
			{
				auto skybox = renderSystem->GetSkybox();

				ryml::NodeRef skyboxNode = envNode["Skybox"];
				bool skyboxEnabled;
				skyboxNode["Enabled"] >> skyboxEnabled;
				skybox->SetEnabled(skyboxEnabled);

				float intensity;
				skyboxNode["Intensity"] >> intensity;
				skybox->SetIntensity(intensity);

				uint64_t texUUID;
				skyboxNode["TextureUUID"] >> texUUID;
				if (texUUID != Constants::InvalidUUID)
				{
					auto& assetManager = Application::Instance().GetAssetManager();
					if (assetManager.ContainsAsset((UUID)texUUID))
						skybox->Initialize((UUID)texUUID);
					else
						EB_CORE_WARN("SceneSerializer: Skybox texture UUID not found in AssetManager. Skybox will not be restored.");
				}
			}

			if (envNode.has_child("Bloom"))
			{
				auto bloomPass = StaticPointerCast<BloomPass>(renderSystem->GetPostProcessPass("BloomPass"));

				ryml::NodeRef bloomNode = envNode["Bloom"];
				bloomNode["Enabled"] >> bloomPass->Enabled;
				bloomNode["Threshold"] >> bloomPass->Settings.Threshold;
				bloomNode["Knee"] >> bloomPass->Settings.Knee;
				bloomNode["Intensity"] >> bloomPass->Settings.Intensity;
				bloomNode["BlurRadius"] >> bloomPass->Settings.BlurRadius;
			}

			if (envNode.has_child("Fog"))
			{
				auto fogPass = StaticPointerCast<FogPass>(renderSystem->GetPostProcessPass("FogPass"));
				ryml::NodeRef fogNode = envNode["Fog"];
				fogNode["Enabled"] >> fogPass->Enabled;
				Util::DeserializeVector3f(fogNode["Color"], fogPass->Settings.Color);
				fogNode["Density"] >> fogPass->Settings.Density;
				fogNode["StartDistance"] >> fogPass->Settings.StartDistance;
				fogNode["Falloff"] >> fogPass->Settings.Falloff;
			}

			if (envNode.has_child("FXAA"))
			{
				auto fxaaPass = StaticPointerCast<FXAAPass>(renderSystem->GetPostProcessPass("FXAAPass"));
				ryml::NodeRef fxaaNode = envNode["FXAA"];
				fxaaNode["Enabled"] >> fxaaPass->Enabled;
				fxaaNode["SubpixelQuality"] >> fxaaPass->SubpixelQuality;
				fxaaNode["EdgeThresholdMin"] >> fxaaPass->EdgeThresholdMin;
				fxaaNode["EdgeThresholdMax"] >> fxaaPass->EdgeThresholdMax;
			}

			if (envNode.has_child("Vignette"))
			{
				auto vignettePass = StaticPointerCast<VignettePass>(renderSystem->GetPostProcessPass("VignettePass"));
				ryml::NodeRef vignetteNode = envNode["Vignette"];
				Util::DeserializeVector3f(vignetteNode["Color"], vignettePass->Settings.Color);
				vignetteNode["Intensity"] >> vignettePass->Settings.Intensity;
				vignetteNode["Smoothness"] >> vignettePass->Settings.Smoothness;
				vignetteNode["Size"] >> vignettePass->Settings.Size;
			}

			if (envNode.has_child("ColorGrading"))
			{
				auto colorGradingPass = StaticPointerCast<ColorGradePass>(renderSystem->GetPostProcessPass("ColorGradePass"));
				auto tonemapPass = StaticPointerCast<ToneMapPass>(renderSystem->GetPostProcessPass("ToneMapPass"));

				ryml::NodeRef colorGradeNode = envNode["ColorGrading"];
				colorGradeNode["Enabled"] >> colorGradingPass->Enabled;
				colorGradeNode["Exposure"] >> tonemapPass->Settings.Exposure;
				colorGradeNode["Temperature"] >> colorGradingPass->Settings.Temperature;
				colorGradeNode["Tint"] >> colorGradingPass->Settings.Tint;
				colorGradeNode["Contrast"] >> colorGradingPass->Settings.Contrast;
				colorGradeNode["Saturation"] >> colorGradingPass->Settings.Saturation;
				Util::DeserializeVector4f(colorGradeNode["Lift"], colorGradingPass->Settings.Lift);
				Util::DeserializeVector4f(colorGradeNode["Gamma"], colorGradingPass->Settings.Gamma);
				Util::DeserializeVector4f(colorGradeNode["Gain"], colorGradingPass->Settings.Gain);
			}
		}

		return true;
	}


	// =========================================================================
	// PREFAB SERIALIZATION
	// =========================================================================
	bool SceneSerializer::SerializePrefab(Entity prefabRoot, const std::string& filepath)
	{
		ryml::Tree tree;
		ryml::NodeRef root = tree.rootref();
		root |= ryml::MAP;

		root["Prefab"] << prefabRoot.GetName();
		ryml::NodeRef entitiesNode = root["PrefabEntities"];
		entitiesNode |= ryml::SEQ;

		// 1. Gather the root and all descendants using a BFS queue
		std::vector<Entity> entitiesToSave;
		entitiesToSave.push_back(prefabRoot);

		for (size_t i = 0; i < entitiesToSave.size(); i++)
		{
			Entity current = entitiesToSave[i];
			if (current.ContainsComponent<RelationshipComponent>())
			{
				for (UUID childUUID : current.GetComponent<RelationshipComponent>().Children)
				{
					Entity child = m_Scene->GetEntity(childUUID);
					if (child != Constants::Entities::InvalidEntityID)
						entitiesToSave.push_back(child);
				}
			}
		}

		// 2. Write them out using our helper
		for (Entity e : entitiesToSave)
		{
			ryml::NodeRef entityNode = entitiesNode.append_child();
			SerializeEntityNode(entityNode, e);
		}

		std::ofstream fout(filepath);
		fout << tree;
		fout.close();

		return true;
	}

	Entity SceneSerializer::DeserializePrefab(SharedPtr<Prefab> prefab, bool preserveUUIDs)
	{
		if (!prefab || prefab->YAMLData.empty())
			return Entity();

		ryml::Tree tree = ryml::parse_in_arena(ryml::to_csubstr(prefab->YAMLData));
		ryml::NodeRef root = tree.rootref();

		if (!root.has_child("PrefabEntities"))
			return Entity();

		ryml::NodeRef entitiesNode = root["PrefabEntities"];

		std::unordered_map<uint64_t, UUID> oldToNewUUIDs;
		std::vector<Entity> newEntities;

		// Create fresh entities and build the Remap Table
		for (ryml::NodeRef entityNode : entitiesNode.children())
		{
			uint64_t oldUUIDVal;
			entityNode["Entity"] >> oldUUIDVal;

			std::string name = "PrefabEntity";
			if (entityNode.has_child("TagComponent"))
				entityNode["TagComponent"]["Tag"] >> name;

			UUID newUUID = preserveUUIDs ? UUID(oldUUIDVal) : UUID();
			oldToNewUUIDs[oldUUIDVal] = newUUID;

			Entity newEntity = m_Scene->AddEntity(newUUID, name);
			newEntities.push_back(newEntity);
		}

		// Deserialize components and repair relationships
		int i = 0;
		for (ryml::NodeRef entityNode : entitiesNode.children())
		{
			DeserializeEntityNode(entityNode, newEntities[i], oldToNewUUIDs);
			i++;
		}

		// Return the root entity
		return newEntities.empty() ? Entity() : newEntities[0];
	}
}