#pragma once

#include <ryml.hpp>
#include <ryml_std.hpp>

namespace Ember {
	namespace Util {
		static void SerializeVector2f(ryml::NodeRef node, const Vector2f& vec)
		{
			node |= ryml::SEQ | ryml::FLOW_SL;
			node.append_child() << vec.x;
			node.append_child() << vec.y;
		}
		static void SerializeVector3f(ryml::NodeRef node, const Vector3f& vec)
		{
			node |= ryml::SEQ | ryml::FLOW_SL;
			node.append_child() << vec.x;
			node.append_child() << vec.y;
			node.append_child() << vec.z;
		}
		static void SerializeVector4f(ryml::NodeRef node, const Vector4f& vec)
		{
			node |= ryml::SEQ | ryml::FLOW_SL;
			node.append_child() << vec.x;
			node.append_child() << vec.y;
			node.append_child() << vec.z;
			node.append_child() << vec.w;
		}

		static void SerializeMatrix4f(ryml::NodeRef node, const Matrix4f& mat)
		{
			node |= ryml::SEQ | ryml::FLOW_SL;
			for (int i = 0; i < 4; i++)
			{
				for (int j = 0; j < 4; j++)
				{
					node.append_child() << mat[i][j];
				}
			}
		}

		static void DeserializeVector2f(ryml::NodeRef node, Vector2f& vec)
		{
			if (node.is_seq() && node.num_children() == 2)
			{
				node[0] >> vec.x;
				node[1] >> vec.y;
			}
		}

		static void DeserializeVector3f(ryml::NodeRef node, Vector3f& vec)
		{
			if (node.is_seq() && node.num_children() == 3)
			{
				node[0] >> vec.x;
				node[1] >> vec.y;
				node[2] >> vec.z;
			}
		}

		static void DeserializeVector4f(ryml::NodeRef node, Vector4f& vec)
		{
			if (node.is_seq() && node.num_children() == 4)
			{
				node[0] >> vec.x;
				node[1] >> vec.y;
				node[2] >> vec.z;
				node[3] >> vec.w;
			}
		}

		static void DeserializeMatrix4f(ryml::NodeRef node, Matrix4f& mat)
		{
			if (node.is_seq() && node.num_children() == 16)
			{
				for (int i = 0; i < 4; i++)
				{
					for (int j = 0; j < 4; j++)
					{
						node[i * 4 + j] >> mat[i][j];
					}
				}
			}
		}

		static std::string GetAssetTypeString(AssetType type)
		{
			switch (type)
			{
			case AssetType::Texture: return "Texture";
			case AssetType::Shader: return "Shader";
			case AssetType::Model: return "Model";
			case AssetType::Material: return "Material";
			case AssetType::Script: return "Script";
			default: return "Unknown";
			}
		}

		static AssetType GetAssetTypeFromString(const std::string& typeStr)
		{
			if (typeStr == "Texture") return AssetType::Texture;
			if (typeStr == "Shader") return AssetType::Shader;
			if (typeStr == "Model") return AssetType::Model;
			if (typeStr == "Material") return AssetType::Material;
			if (typeStr == "Script") return AssetType::Script;
			return AssetType::Texture; // Default to Texture if unknown
		}

		static void SerializeGeneralAsset(ryml::NodeRef node, const SharedPtr<Asset>& asset)
		{
			node |= ryml::MAP;
			node["Type"] << GetAssetTypeString(asset->GetType());
			node["UUID"] << asset->GetUUID();
			node["Name"] << asset->GetName();
			node["FilePath"] << asset->GetFilePath();
		}

		//static void SerializeModel(ryml::NodeRef node, const SharedPtr<Model>& model)
		//{
		//	SerializeGeneralAsset(node, model); // Saves UUID, Name, FilePath

		//	// Save the ordered list of Mesh UUIDs
		//	ryml::NodeRef meshesNode = node["Meshes"];
		//	meshesNode |= ryml::SEQ;
		//	for (const auto& meshNode : model->GetAllMeshes())
		//	{
		//		meshesNode.append_child() << meshNode.MeshAsset->GetUUID();
		//	}

		//	// Save the ordered list of Material UUIDs
		//	ryml::NodeRef materialsNode = node["Materials"];
		//	materialsNode |= ryml::SEQ;
		//	for (const auto& material : model->GetAllMaterials())
		//	{
		//		materialsNode.append_child() << material->GetUUID();
		//	}
		//}

		static void SerializeMaterial(ryml::NodeRef node, const SharedPtr<MaterialBase>& material)
		{
			SerializeGeneralAsset(node, material);
			node["RenderQueue"] << static_cast<int>(material->GetRenderQueue());
			node["Shader"] << material->GetShader()->GetUUID();

			// See if instanced
			auto instance = DynamicPointerCast<MaterialInstance>(material);
			bool isInstanced = instance != nullptr;
			node["Instanced"] << (isInstanced ? "true" : "false");

			if (isInstanced)
			{
				if (instance->GetMaterial())
					node["BaseMaterialUUID"] << instance->GetMaterial()->GetUUID();
				else
					node["BaseMaterialUUID"] << Constants::InvalidUUID;
			}

			ryml::NodeRef uniformsNode = node["Uniforms"];
			uniformsNode |= ryml::MAP;

			for (const auto& [name, value] : material->GetUniforms())
			{
				ryml::NodeRef uniformNode = uniformsNode[name.c_str()];
				uniformNode |= ryml::MAP;

				std::visit([&](auto&& arg) {
					using T = std::decay_t<decltype(arg)>;

					if constexpr (std::is_same_v<T, int>)
					{
						uniformNode["Type"] << "int";
						uniformNode["Value"] << arg;
					}
					else if constexpr (std::is_same_v<T, float>)
					{
						uniformNode["Type"] << "float";
						uniformNode["Value"] << arg;
					}
					else if constexpr (std::is_same_v<T, Vector2f>)
					{
						uniformNode["Type"] << "Vector2f";
						SerializeVector2f(uniformNode["Value"], arg);
					}
					else if constexpr (std::is_same_v<T, Vector3f>)
					{
						uniformNode["Type"] << "Vector3f";
						SerializeVector3f(uniformNode["Value"], arg);
					}
					else if constexpr (std::is_same_v<T, Vector4f>)
					{
						uniformNode["Type"] << "Vector4f";
						SerializeVector4f(uniformNode["Value"], arg);
					}
					else if constexpr (std::is_same_v<T, Matrix4f>)
					{
						uniformNode["Type"] << "Matrix4f";
						SerializeMatrix4f(uniformNode["Value"], arg);
					}
					else if constexpr (std::is_same_v<T, SharedPtr<Texture>>)
					{
						uniformNode["Type"] << "Texture";
						if (arg)
							uniformNode["Value"] << arg->GetUUID();
						else
							uniformNode["Value"] << Constants::InvalidUUID;
					}
				}, value);
			}
		}

		static SharedPtr<MaterialBase> DeserializeMaterial(ryml::NodeRef node, AssetManager* assetManager)
		{
			uint64_t materialUUID, shaderUUID;
			node["UUID"] >> materialUUID;
			node["Shader"] >> shaderUUID;

			std::string name;
			node["Name"] >> name;

			int renderQueue;
			node["RenderQueue"] >> renderQueue;

			std::string instancedStr = "false";
			if (node.has_child("Instanced"))
				node["Instanced"] >> instancedStr;

			SharedPtr<MaterialBase> material;

			if (instancedStr == "true")
			{
				uint64_t baseMaterialUUID = 0;
				if (node.has_child("BaseMaterialUUID"))
					node["BaseMaterialUUID"] >> baseMaterialUUID;

				SharedPtr<Material> baseMaterial = assetManager->GetAsset<Material>(baseMaterialUUID);

				if (!baseMaterial)
				{
					EB_CORE_WARN("Could not find Base Material for instance '{0}'. Falling back.", name);
					baseMaterial = assetManager->GetAsset<Material>(Constants::Assets::StandardGeometryMat);
				}

				material = SharedPtr<MaterialInstance>::Create(materialUUID, name, baseMaterial);
			}
			else
			{
				SharedPtr<Shader> shader = assetManager->GetAsset<Shader>(shaderUUID);
				if (!shader)
					shader = assetManager->GetAsset<Shader>(Constants::Assets::StandardGeometryShad);

				material = SharedPtr<Material>::Create(materialUUID, name, shader, static_cast<RenderQueue>(renderQueue));
			}

			if (node.has_child("Uniforms"))
			{
				ryml::NodeRef uniformsNode = node["Uniforms"];
				for (ryml::NodeRef uniformNode : uniformsNode.children())
				{
					c4::csubstr keyStr = uniformNode.key();
					std::string uniformName(keyStr.str, keyStr.len);

					std::string typeStr;
					uniformNode["Type"] >> typeStr;

					ryml::NodeRef valNode = uniformNode["Value"];

					if (typeStr == "int")
					{
						int val; valNode >> val;
						material->SetUniform(uniformName, val);
					}
					else if (typeStr == "float")
					{
						float val; valNode >> val;
						material->SetUniform(uniformName, val);
					}
					else if (typeStr == "Vector2f")
					{
						Vector2f val; DeserializeVector2f(valNode, val);
						material->SetUniform(uniformName, val);
					}
					else if (typeStr == "Vector3f")
					{
						Vector3f val; DeserializeVector3f(valNode, val);
						material->SetUniform(uniformName, val);
					}
					else if (typeStr == "Vector4f")
					{
						Vector4f val; DeserializeVector4f(valNode, val);
						material->SetUniform(uniformName, val);
					}
					else if (typeStr == "Matrix4f")
					{
						Matrix4f val; DeserializeMatrix4f(valNode, val);
						material->SetUniform(uniformName, val);
					}
					else if (typeStr == "Texture")
					{
						uint64_t texUUID;
						valNode >> texUUID;

						SharedPtr<Texture> tex = assetManager->GetAsset<Texture>(texUUID);
						if (tex)
							material->SetUniform(uniformName, tex);
					}
				}
			}

			return material;
		}
	}
}