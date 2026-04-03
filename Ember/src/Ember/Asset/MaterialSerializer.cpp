#include "ebpch.h"
#include "MaterialSerializer.h"

#include "AssetManager.h"

namespace Ember {

	SharedPtr<MaterialInstance> MaterialSerializer::Deserialize(UUID uuid, const std::filesystem::path& filepath, AssetManager& assetManager)
	{
		std::ifstream file(filepath);
		if (!file.is_open())
		{
			EB_CORE_ERROR("Failed to open cooked material file: {0}", filepath.string());
			return nullptr;
		}

		std::stringstream buffer;
		buffer << file.rdbuf();
		std::string yamlStr = buffer.str();
		file.close();

		ryml::Tree tree = ryml::parse_in_arena(ryml::to_csubstr(yamlStr));
		ryml::NodeRef root = tree.rootref();

		if (!root.has_child("Material"))
		{
			EB_CORE_ERROR("Invalid material file format: {0}", filepath.string());
			return nullptr;
		}

		std::string matName, baseMatName;
		root["Material"] >> matName;
		root["BaseMaterial"] >> baseMatName;

		auto baseMat = assetManager.GetAsset<Material>(baseMatName);
		if (!baseMat)
		{
			EB_CORE_ERROR("Could not find BaseMaterial '{0}' for '{1}'", baseMatName, matName);
			return nullptr;
		}

		auto instance = SharedPtr<MaterialInstance>::Create(uuid, matName, baseMat);

		// Set Uniforms
		if (root.has_child("Uniforms"))
		{
			ryml::NodeRef uniforms = root["Uniforms"];

			if (uniforms.has_child("Albedo"))
			{
				Vector3f albedo;
				int i = 0;
				for (ryml::NodeRef child : uniforms["Albedo"].children())
					child >> albedo[i++];

				instance->SetUniform(Constants::Uniforms::Albedo, albedo);
				instance->SetUniform(Constants::Uniforms::Color, albedo); // In case it's unlit
			}

			if (uniforms.has_child("Emission"))
			{
				float emission; uniforms["Emission"] >> emission;
				instance->SetUniform(Constants::Uniforms::Emission, emission);
			}

			if (uniforms.has_child("Roughness"))
			{
				float roughness; uniforms["Roughness"] >> roughness;
				instance->SetUniform(Constants::Uniforms::Roughness, roughness);
			}

			if (uniforms.has_child("Metallic"))
			{
				float metallic; uniforms["Metallic"] >> metallic;
				instance->SetUniform(Constants::Uniforms::Metallic, metallic);
			}
		}

		// Load Textures
		if (root.has_child("Textures"))
		{
			ryml::NodeRef textures = root["Textures"];
			std::filesystem::path dir = filepath.parent_path();

			// Helper lambda to load a texture if it exists in the YAML
			auto loadTexture = [&](const char* yamlKey, const std::string& uniformName) {
				if (textures.has_child(yamlKey))
				{
					std::string texFileName;
					textures[yamlKey] >> texFileName;

					// Reconstruct absolute path relative to the .ebmat file!
					std::string texPath = (dir / texFileName).string();
					auto tex = assetManager.Load<Texture>(texPath);

					if (tex) instance->SetUniform(uniformName, tex);
				}
				};

			loadTexture("AlbedoMap", Constants::Uniforms::AlbedoMap);
			loadTexture("NormalMap", Constants::Uniforms::NormalMap);
			loadTexture("EmissiveMap", Constants::Uniforms::EmissiveMap);
			loadTexture("ORMMap", Constants::Uniforms::MetallicRoughnessMap);
		}

		return instance;
	}
}