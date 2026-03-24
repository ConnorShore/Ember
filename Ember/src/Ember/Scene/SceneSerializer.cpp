#include "ebpch.h"
#include "SceneSerializer.h"
#include "Ember/ECS/Component/Components.h"
#include "Ember/Utils/SerializationUtils.h"
#include "Ember/Asset/ScriptRegistry.h"

#include <ryml.hpp>
#include <ryml_std.hpp>
#include <fstream>
#include <sstream>

namespace Ember {
	
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
				if (entity.GetComponent<SpriteComponent>().Texture)
				{
					spriteNode["Texture"] << entity.GetComponent<SpriteComponent>().Texture->GetName();
				}
			}
			if (entity.ContainsComponent<RigidBodyComponent>())
			{
				ryml::NodeRef rigidBodyNode = entityNode["RigidBodyComponent"];
				rigidBodyNode |= ryml::MAP;
				Util::SerializeVector3f(rigidBodyNode["Velocity"], entity.GetComponent<RigidBodyComponent>().Velocity);
			}
			if (entity.ContainsComponent<MeshComponent>())
			{
				ryml::NodeRef meshNode = entityNode["MeshComponent"];
				meshNode |= ryml::MAP;
				if (entity.GetComponent<MeshComponent>().Mesh)
				{
					meshNode["Mesh"] << entity.GetComponent<MeshComponent>().Mesh->GetName();
				}
			}
			if (entity.ContainsComponent<MaterialComponent>())
			{
				ryml::NodeRef materialNode = entityNode["MaterialComponent"];
				materialNode |= ryml::MAP;
				if (entity.GetComponent<MaterialComponent>().Material)
				{
					materialNode["Material"] << entity.GetComponent<MaterialComponent>().Material->GetName();
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
				ryml::NodeRef scNode = entityNode["ScriptComponent"];
				scNode |= ryml::MAP;

				auto& sc = entity.GetComponent<ScriptComponent>();
				scNode["ClassName"] << sc.ClassName;
			}
			if (entity.ContainsComponent<OutlineComponent>())
			{
				auto& outlineComp = entity.GetComponent<OutlineComponent>();
				ryml::NodeRef outlineNode = entityNode["OutlineComponent"];
				outlineNode |= ryml::MAP;
				Util::SerializeVector3f(outlineNode["Color"], outlineComp.Color);
				outlineNode["Thickness"] << outlineComp.Thickness;
			}
		}

		// Write out to disk
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

			for (ryml::NodeRef entityNode : entitiesNode.children())
			{
				uint64_t uuidVal;
				entityNode["Entity"] >> uuidVal;
				UUID uuid = UUID(uuidVal);

				std::string name = "Entity";
				if (entityNode.has_child("TagComponent"))
					entityNode["TagComponent"]["Tag"] >> name;

				// AddEntity automatically attaches ID, Tag, Transform, and Relationship components
				Entity deserializedEntity = m_Scene->AddEntity(uuid, name);

				// Core components are already attached, this just updates them
				if (entityNode.has_child("RelationshipComponent"))
				{
					auto& rc = deserializedEntity.GetComponent<RelationshipComponent>();
					ryml::NodeRef rcNode = entityNode["RelationshipComponent"];

					uint64_t parentVal;
					rcNode["Parent"] >> parentVal;
					rc.ParentHandle = UUID(parentVal);

					if (rcNode.has_child("Children"))
					{
						for (ryml::NodeRef childNode : rcNode["Children"].children())
						{
							uint64_t childVal;
							childNode >> childVal;
							rc.Children.push_back(UUID(childVal));
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

					SpriteComponent sc(color);
					if (spriteNode.has_child("Texture"))
					{
						std::string textureName;
						spriteNode["Texture"] >> textureName;
						sc.Texture = m_Scene->GetAsset<Texture>(textureName);
					}
					deserializedEntity.AttachComponent<SpriteComponent>(sc);
				}

				if (entityNode.has_child("RigidBodyComponent"))
				{
					ryml::NodeRef rbNode = entityNode["RigidBodyComponent"];
					Vector3f velocity;
					Util::DeserializeVector3f(rbNode["Velocity"], velocity);

					RigidBodyComponent rbc(velocity);
					deserializedEntity.AttachComponent<RigidBodyComponent>(rbc);
				}

				if (entityNode.has_child("MeshComponent"))
				{
					ryml::NodeRef meshNode = entityNode["MeshComponent"];
					MeshComponent mc;
					if (meshNode.has_child("Mesh"))
					{
						std::string meshName;
						meshNode["Mesh"] >> meshName;
						mc.Mesh = m_Scene->GetAsset<Mesh>(meshName);
					}
					deserializedEntity.AttachComponent<MeshComponent>(mc);
				}

				// TODO: FIx material loading
				if (entityNode.has_child("MaterialComponent"))
				{
					ryml::NodeRef materialNode = entityNode["MaterialComponent"];
					MaterialComponent matComp;
					if (materialNode.has_child("Material"))
					{
						std::string matName;
						materialNode["Material"] >> matName;

						matComp.Material = m_Scene->GetAsset<MaterialBase>(matName);

						if (!matComp.Material)
						{
							EB_CORE_ERROR("Deserializer failed to load Material: {0}. Assigning Fallback.", matName);
							matComp.Material = m_Scene->GetAsset<MaterialBase>(Constants::Assets::DefaultMat);
						}
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
					ryml::NodeRef scriptNode = entityNode["ScriptComponent"];
					std::string className;
					scriptNode["ClassName"] >> className;

					auto& sc = deserializedEntity.AttachComponent<ScriptComponent>();
					sc.ClassName = className;

					ScriptRegistry::ScriptFunctions functions;
					if (ScriptRegistry::GetScriptFunctions(className, functions))
					{
						sc.OnInitFunc = functions.Instantiate;
						sc.OnDestroyFunc = functions.Destroy;
					}
					else
					{
						EB_CORE_WARN("Failed to deserialize script! Class '{}' is not registered.", className);
					}
				}

				if (entityNode.has_child("OutlineComponent"))
				{
					ryml::NodeRef outlineNode = entityNode["OutlineComponent"];
					OutlineComponent oc;
					Util::DeserializeVector3f(outlineNode["Color"], oc.Color);
					outlineNode["Thickness"] >> oc.Thickness;
					deserializedEntity.AttachComponent<OutlineComponent>(oc);
				}
			}
		}

		return true;
	}

}