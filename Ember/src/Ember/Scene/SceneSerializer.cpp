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

#include <ryml.hpp>
#include <ryml_std.hpp>
#include <string>
#include <fstream>
#include <sstream>
#include <queue>

namespace Ember {

	// =========================================================================
	// HELPER: SERIALIZE SINGLE ENTITY
	// =========================================================================
	void SceneSerializer::SerializeEntityNode(ryml::NodeRef& entityNode, Entity entity)
	{
		entityNode |= ryml::MAP;
		entityNode["Entity"] << (uint64_t)entity.GetComponent<IDComponent>().ID;

		if (entity.ContainsComponent<TagComponent>())
		{
			ryml::NodeRef tagNode = entityNode["TagComponent"];
			tagNode |= ryml::MAP;
			tagNode["Tag"] << entity.GetComponent<TagComponent>().Tag;
		}
		if (entity.ContainsComponent<RelationshipComponent>())
		{
			ryml::NodeRef relationshipNode = entityNode["RelationshipComponent"];
			relationshipNode |= ryml::MAP;
			relationshipNode["Parent"] << (uint64_t)entity.GetComponent<RelationshipComponent>().ParentHandle;
			ryml::NodeRef childrenNode = relationshipNode["Children"];
			childrenNode |= ryml::SEQ;
			for (const auto& child : entity.GetComponent<RelationshipComponent>().Children)
			{
				childrenNode.append_child() << (uint64_t)child;
			}
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
			ryml::NodeRef spriteNode = entityNode["SpriteComponent"];
			spriteNode |= ryml::MAP;
			Util::SerializeVector4f(spriteNode["Color"], entity.GetComponent<SpriteComponent>().Color);
			if (entity.GetComponent<SpriteComponent>().TextureHandle != Constants::InvalidUUID)
			{
				spriteNode["TextureUUID"] << entity.GetComponent<SpriteComponent>().TextureHandle;
			}
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
			}
		}
		if (entity.ContainsComponent<StaticMeshComponent>())
		{
			ryml::NodeRef meshNode = entityNode["StaticMeshComponent"];
			meshNode |= ryml::MAP;
			if (entity.GetComponent<StaticMeshComponent>().MeshHandle != Constants::InvalidUUID)
			{
				meshNode["MeshUUID"] << entity.GetComponent<StaticMeshComponent>().MeshHandle;
			}
		}
		if (entity.ContainsComponent<SkinnedMeshComponent>())
		{
			ryml::NodeRef meshNode = entityNode["SkinnedMeshComponent"];
			meshNode |= ryml::MAP;
			if (entity.GetComponent<SkinnedMeshComponent>().MeshHandle != Constants::InvalidUUID)
			{
				meshNode["MeshUUID"] << entity.GetComponent<SkinnedMeshComponent>().MeshHandle;
				meshNode["RootAnimator"] << (uint64_t)entity.GetComponent<SkinnedMeshComponent>().AnimatorEntityHandle;
			}
		}
		if (entity.ContainsComponent<MaterialComponent>())
		{
			ryml::NodeRef materialNode = entityNode["MaterialComponent"];
			materialNode |= ryml::MAP;
			if (entity.GetComponent<MaterialComponent>().MaterialHandle != Constants::InvalidUUID)
			{
				materialNode["MaterialUUID"] << entity.GetComponent<MaterialComponent>().MaterialHandle;
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
			Util::SerializeVector3f(lightNode["Color"], lightComp.Color);
			lightNode["Intensity"] << lightComp.Intensity;
		}
		if (entity.ContainsComponent<SpotLightComponent>())
		{
			auto& lightComp = entity.GetComponent<SpotLightComponent>();
			ryml::NodeRef lightNode = entityNode["SpotLightComponent"];
			lightNode |= ryml::MAP;
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
					propNode["Value"] << std::get<int>(prop.Value);
					break;
				case ScriptPropertyType::Bool:
					propNode["Value"] << std::get<bool>(prop.Value);
					break;
				case ScriptPropertyType::String:
					propNode["Value"] << std::get<std::string>(prop.Value);
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
		if (entity.ContainsComponent<BillboardComponent>())
		{
			auto& billboard = entity.GetComponent<BillboardComponent>();
			ryml::NodeRef billboardNode = entityNode["BillboardComponent"];
			billboardNode |= ryml::MAP;
			billboardNode["TextureHandle"] << (uint64_t)billboard.TextureHandle;
			Util::SerializeVector4f(billboardNode["Tint"], billboard.Tint);
			billboardNode["Spherical"] << billboard.Spherical;
			billboardNode["StaticSize"] << billboard.StaticSize;
			billboardNode["Size"] << billboard.Size;
			billboardNode["RenderRuntime"] << billboard.RenderRuntime;
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
		}
		if (entity.ContainsComponent<TextComponent>())
		{
			auto& text = entity.GetComponent<TextComponent>();
			ryml::NodeRef textNode = entityNode["TextComponent"];
			textNode |= ryml::MAP;
			textNode["Text"] << text.Text;
			textNode["FontHandle"] << (uint64_t)text.FontHandle;
			Util::SerializeVector4f(textNode["Color"], text.Color);
			textNode["ScreenSpace"] << text.ScreenSpace;
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

			uint64_t texId;
			spriteNode["TextureUUID"] >> texId;

			SpriteComponent sc(color);
			sc.TextureHandle = (UUID)texId;
			deserializedEntity.AttachComponent<SpriteComponent>(sc);
		}

		if (entityNode.has_child("RigidBodyComponent"))
		{
			ryml::NodeRef rbNode = entityNode["RigidBodyComponent"];

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
			BoxColliderComponent bcc;
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
			uint64_t bccPhysMatId;
			colliderNode["PhysicsMaterialUUID"] >> bccPhysMatId;
			bcc.PhysicsMaterialHandle = (UUID)bccPhysMatId;
			deserializedEntity.AttachComponent<BoxColliderComponent>(bcc);
		}

		if (entityNode.has_child("SphereColliderComponent"))
		{
			ryml::NodeRef colliderNode = entityNode["SphereColliderComponent"];
			SphereColliderComponent scc;
			colliderNode["Radius"] >> scc.Radius;
			colliderNode["IsTrigger"] >> scc.IsTrigger;
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
			deserializedEntity.AttachComponent<SphereColliderComponent>(scc);
		}

		if (entityNode.has_child("CapsuleColliderComponent"))
		{
			ryml::NodeRef colliderNode = entityNode["CapsuleColliderComponent"];
			CapsuleColliderComponent ccc;
			colliderNode["Radius"] >> ccc.Radius;
			colliderNode["Height"] >> ccc.Height;
			colliderNode["IsTrigger"] >> ccc.IsTrigger;
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
			deserializedEntity.AttachComponent<CapsuleColliderComponent>(ccc);
		}

		if (entityNode.has_child("ConvexMeshColliderComponent"))
		{
			ryml::NodeRef colliderNode = entityNode["ConvexMeshColliderComponent"];
			uint64_t meshId;
			colliderNode["MeshUUID"] >> meshId;
			UUID meshUUID = (UUID)meshId;

			ConvexMeshColliderComponent ccc;
			ccc.MeshHandle = meshUUID;
			colliderNode["IsTrigger"] >> ccc.IsTrigger;
			if (colliderNode.has_child("OffsetPosition"))
				Util::DeserializeVector3f(colliderNode["OffsetPosition"], ccc.Offset.Position);
			if (colliderNode.has_child("OffsetRotation"))
				Util::DeserializeVector3f(colliderNode["OffsetRotation"], ccc.Offset.Rotation);
			colliderNode["Category"] >> ccc.Category;
			colliderNode["CollisionMask"] >> ccc.CollisionMask;
			uint64_t convexPhysMatId;
			colliderNode["PhysicsMaterialUUID"] >> convexPhysMatId;
			ccc.PhysicsMaterialHandle = (UUID)convexPhysMatId;
			deserializedEntity.AttachComponent<ConvexMeshColliderComponent>(ccc);
		}

		if (entityNode.has_child("ConcaveMeshColliderComponent"))
		{
			ryml::NodeRef colliderNode = entityNode["ConcaveMeshColliderComponent"];
			uint64_t meshId;
			colliderNode["MeshUUID"] >> meshId;
			UUID meshUUID = (UUID)meshId;

			ConcaveMeshColliderComponent cmcc;
			cmcc.MeshHandle = meshUUID;
			colliderNode["IsTrigger"] >> cmcc.IsTrigger;
			if (colliderNode.has_child("OffsetPosition"))
				Util::DeserializeVector3f(colliderNode["OffsetPosition"], cmcc.Offset.Position);
			if (colliderNode.has_child("OffsetRotation"))
				Util::DeserializeVector3f(colliderNode["OffsetRotation"], cmcc.Offset.Rotation);
			colliderNode["Category"] >> cmcc.Category;
			colliderNode["CollisionMask"] >> cmcc.CollisionMask;
			uint64_t concavePhysMatId;
			colliderNode["PhysicsMaterialUUID"] >> concavePhysMatId;
			cmcc.PhysicsMaterialHandle = (UUID)concavePhysMatId;
			deserializedEntity.AttachComponent<ConcaveMeshColliderComponent>(cmcc);
		}

		if (entityNode.has_child("StaticMeshComponent"))
		{
			ryml::NodeRef meshNode = entityNode["StaticMeshComponent"];
			uint64_t meshId;
			meshNode["MeshUUID"] >> meshId;

			StaticMeshComponent mc;
			mc.MeshHandle = (UUID)meshId;
			deserializedEntity.AttachComponent<StaticMeshComponent>(mc);
		}

		if (entityNode.has_child("SkinnedMeshComponent"))
		{
			ryml::NodeRef meshNode = entityNode["SkinnedMeshComponent"];
			uint64_t meshId;
			meshNode["MeshUUID"] >> meshId;

			uint64_t animatorId;
			meshNode["RootAnimator"] >> animatorId;

			SkinnedMeshComponent mc;
			mc.MeshHandle = (UUID)meshId;
			mc.AnimatorEntityHandle = getRemappedUUID(animatorId); // REMAPPED!
			deserializedEntity.AttachComponent<SkinnedMeshComponent>(mc);
		}

		if (entityNode.has_child("MaterialComponent"))
		{
			ryml::NodeRef materialNode = entityNode["MaterialComponent"];

			uint64_t materialId;
			materialNode["MaterialUUID"] >> materialId;

			MaterialComponent matComp;
			matComp.MaterialHandle = (UUID)materialId;
			if (matComp.MaterialHandle == Constants::InvalidUUID)
			{
				EB_CORE_ERROR("Deserializer failed to load Material. Assigning Fallback.");
				matComp.MaterialHandle = Constants::Assets::DefaultMatUUID;
			}
			deserializedEntity.AttachComponent<MaterialComponent>(matComp);
		}

		if (entityNode.has_child("CameraComponent"))
		{
			ryml::NodeRef cameraNode = entityNode["CameraComponent"];
			CameraComponent cc;

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

			deserializedEntity.AttachComponent<CameraComponent>(cc);
		}

		if (entityNode.has_child("DirectionalLightComponent"))
		{
			ryml::NodeRef lightNode = entityNode["DirectionalLightComponent"];
			DirectionalLightComponent dlc;
			Util::DeserializeVector3f(lightNode["Color"], dlc.Color);
			lightNode["Intensity"] >> dlc.Intensity;
			deserializedEntity.AttachComponent<DirectionalLightComponent>(dlc);
		}

		if (entityNode.has_child("SpotLightComponent"))
		{
			ryml::NodeRef lightNode = entityNode["SpotLightComponent"];
			SpotLightComponent slc;
			Util::DeserializeVector3f(lightNode["Color"], slc.Color);
			lightNode["Intensity"] >> slc.Intensity;

			lightNode["CutOffAngle"] >> slc.CutOffAngle;
			slc.CutOff = std::cos(slc.CutOffAngle);

			lightNode["OuterCutOffAngle"] >> slc.OuterCutOffAngle;
			slc.OuterCutOff = std::cos(slc.OuterCutOffAngle);

			deserializedEntity.AttachComponent<SpotLightComponent>(slc);
		}

		if (entityNode.has_child("PointLightComponent"))
		{
			ryml::NodeRef lightNode = entityNode["PointLightComponent"];
			PointLightComponent plc;
			Util::DeserializeVector3f(lightNode["Color"], plc.Color);
			lightNode["Intensity"] >> plc.Intensity;
			lightNode["Radius"] >> plc.Radius;
			deserializedEntity.AttachComponent<PointLightComponent>(plc);
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
					default:
						break;
					}

					scriptUserOverrides[propName] = { propName, propValue, propType };
				}
			}

			ScriptComponent sc((UUID)uuidVal);
			sc.UserPropertyOverrides = scriptUserOverrides;
			deserializedEntity.AttachComponent<ScriptComponent>(sc);
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
			AnimatorComponent ac;

			uint64_t skelHandle = Constants::InvalidUUID;
			if (animatorNode.has_child("SkeletonHandle"))
				animatorNode["SkeletonHandle"] >> skelHandle;

			uint64_t animHandle = Constants::InvalidUUID;
			if (animatorNode.has_child("CurrentAnimationHandle"))
				animatorNode["CurrentAnimationHandle"] >> animHandle;

			ac.SkeletonHandle = (UUID)skelHandle;
			ac.CurrentAnimationHandle = (UUID)animHandle;

			deserializedEntity.AttachComponent<AnimatorComponent>(ac);
		}

		if (entityNode.has_child("BillboardComponent"))
		{
			ryml::NodeRef billboardNode = entityNode["BillboardComponent"];
			BillboardComponent bc;

			uint64_t texHandle;
			billboardNode["TextureHandle"] >> texHandle;
			bc.TextureHandle = (UUID)texHandle;

			Util::DeserializeVector4f(billboardNode["Tint"], bc.Tint);

			billboardNode["Spherical"] >> bc.Spherical;
			billboardNode["StaticSize"] >> bc.StaticSize;
			billboardNode["Size"] >> bc.Size;
			billboardNode["RenderRuntime"] >> bc.RenderRuntime;

			deserializedEntity.AttachComponent<BillboardComponent>(bc);
		}

		if (entityNode.has_child("PrefabComponent"))
		{
			ryml::NodeRef prefabNode = entityNode["PrefabComponent"];
			uint64_t prefabId;
			prefabNode["PrefabUUID"] >> prefabId;
			PrefabComponent pc;
			pc.PrefabHandle = (UUID)prefabId;
			deserializedEntity.AttachComponent<PrefabComponent>(pc);
		}

		if (entityNode.has_child("CharacterControllerComponent"))
		{
			ryml::NodeRef controllerNode = entityNode["CharacterControllerComponent"];
			CharacterControllerComponent ccc;
			controllerNode["WalkSpeed"] >> ccc.WalkSpeed;
			controllerNode["JumpForce"] >> ccc.JumpForce;
			controllerNode["GravityMultiplier"] >> ccc.GravityMultiplier;
			controllerNode["MaxSlopeAngle"] >> ccc.MaxSlopeAngle;
			controllerNode["MaxStepHeight"] >> ccc.MaxStepHeight;
			deserializedEntity.AttachComponent<CharacterControllerComponent>(ccc);
		}

		if (entityNode.has_child("LifetimeComponent"))
		{
			ryml::NodeRef lifetimeNode = entityNode["LifetimeComponent"];
			LifetimeComponent ltc;
			lifetimeNode["Lifetime"] >> ltc.Lifetime;
			deserializedEntity.AttachComponent<LifetimeComponent>(ltc);
		}
		
		if (entityNode.has_child("TextComponent"))
		{
			ryml::NodeRef textNode = entityNode["TextComponent"];
			TextComponent tc;
			textNode["Text"] >> tc.Text;
			uint64_t fontId;
			textNode["FontHandle"] >> fontId;
			tc.FontHandle = (UUID)fontId;
			Util::DeserializeVector4f(textNode["Color"], tc.Color);
			textNode["ScreenSpace"] >> tc.ScreenSpace;
			deserializedEntity.AttachComponent<TextComponent>(tc);
		}

		if (entityNode.has_child("DisabledComponent"))
		{
			// No props just attach the component to mark the entity as disabled

			DisabledComponent dc;
			deserializedEntity.AttachComponent<DisabledComponent>(dc);
		}

		if (entityNode.has_child("PoolComponent"))
		{
			ryml::NodeRef poolNode = entityNode["PoolComponent"];

			PoolComponent pc;
			poolNode["PoolID"] >> pc.PoolID;

			deserializedEntity.AttachComponent<PoolComponent>(pc);
		}

		if (entityNode.has_child("PoolConfigComponent"))
		{
			ryml::NodeRef poolNode = entityNode["PoolConfigComponent"];

			PoolConfigComponent pcc;
			poolNode["PoolID"] >> pcc.PoolID;
			poolNode["Capacity"] >> pcc.Capacity;

			uint64_t prefabId;
			poolNode["PrefabHandle"] >> prefabId;
			pcc.PrefabHandle = (UUID)prefabId;

			deserializedEntity.AttachComponent<PoolConfigComponent>(pcc);
		}

		if (entityNode.has_child("ParticleEmitterComponent"))
		{
			ryml::NodeRef emitterNode = entityNode["ParticleEmitterComponent"];
			ParticleEmitterComponent pec;
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
			deserializedEntity.AttachComponent<ParticleEmitterComponent>(pec);
		}
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

		auto view = m_Scene->GetRegistry().Query<IDComponent>();
		for (auto entityID : view)
		{
			Entity entity = { entityID, m_Scene.Ptr() };
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
		bloomNode["Threshold"] << bloomPass->Threshold;
		bloomNode["Knee"] << bloomPass->Knee;
		bloomNode["Intensity"] << bloomPass->Intensity;
		bloomNode["BlurRadius"] << bloomPass->BlurRadius;

		ryml::NodeRef fxaaNode = envNode["FXAA"];
		fxaaNode |= ryml::MAP;
		fxaaNode["Enabled"] << fxaaPass->Enabled;
		fxaaNode["SubpixelQuality"] << fxaaPass->SubpixelQuality;
		fxaaNode["EdgeThresholdMin"] << fxaaPass->EdgeThresholdMin;
		fxaaNode["EdgeThresholdMax"] << fxaaPass->EdgeThresholdMax;

		ryml::NodeRef colorGradeNode = envNode["ColorGrading"];
		colorGradeNode |= ryml::MAP;
		colorGradeNode["Enabled"] << colorGradingPass->Enabled;
		colorGradeNode["Exposure"] << toneMapPass->Exposure;
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
				bloomNode["Threshold"] >> bloomPass->Threshold;
				bloomNode["Knee"] >> bloomPass->Knee;
				bloomNode["Intensity"] >> bloomPass->Intensity;
				bloomNode["BlurRadius"] >> bloomPass->BlurRadius;
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

			if (envNode.has_child("ColorGrading"))
			{
				auto colorGradingPass = StaticPointerCast<ColorGradePass>(renderSystem->GetPostProcessPass("ColorGradePass"));
				auto tonemapPass = StaticPointerCast<ToneMapPass>(renderSystem->GetPostProcessPass("ToneMapPass"));

				ryml::NodeRef colorGradeNode = envNode["ColorGrading"];
				colorGradeNode["Enabled"] >> colorGradingPass->Enabled;
				colorGradeNode["Exposure"] >> tonemapPass->Exposure;
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

	Entity SceneSerializer::DeserializePrefab(SharedPtr<Prefab> prefab)
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

			// Generate a brand new UUID for this scene!
			UUID newUUID = UUID();
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