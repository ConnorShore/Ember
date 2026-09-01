#pragma once

#include <ryml.hpp>
#include <ryml_std.hpp>

#include "Ember/Asset/Asset.h"
#include "Ember/Render/Material.h"
#include "Ember/Render/Texture2D.h"
#include "Ember/Asset/AssetManager.h"
#include "Ember/Math/Math.h"
#include "Ember/Utils/YamlUtils.h"

namespace Ember {
	namespace Util {
		inline static void SerializeGeneralAsset(ryml::NodeRef node, const SharedPtr<Asset>& asset)
		{
			node |= ryml::MAP;
			node["Type"] << GetAssetTypeString(asset->GetType());
			node["UUID"] << asset->GetUUID();
			node["Name"] << asset->GetName();
			node["FilePath"] << asset->GetFilePath();
		}

		inline static void SerializeMaterial(ryml::NodeRef node, const SharedPtr<MaterialBase>& material)
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
					else if constexpr (std::is_same_v<T, SharedPtr<Texture2D>>)
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
			ReadField(node, "Instanced", instancedStr);

			SharedPtr<MaterialBase> material;

			if (instancedStr == "true")
			{
				uint64_t baseMaterialUUID = 0;
				ReadField(node, "BaseMaterialUUID", baseMaterialUUID);

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

						SharedPtr<Texture2D> tex = assetManager->GetAsset<Texture2D>(texUUID);
						if (tex)
							material->SetUniform(uniformName, tex);
					}
				}
			}

			return material;
		}
	}
}