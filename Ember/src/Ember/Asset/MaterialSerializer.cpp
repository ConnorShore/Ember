#include "ebpch.h"
#include "MaterialSerializer.h"
#include "AssetManager.h"
#include "Ember/Utils/SerializationUtils.h"

namespace Ember {

	bool MaterialSerializer::Serialize(const std::filesystem::path& filepath, const SharedPtr<MaterialBase>& material)
	{
		ryml::Tree tree;
		ryml::NodeRef root = tree.rootref();
		root |= ryml::MAP;

		root["Material"] << material->GetName();
		root["UUID"] << (uint64_t)material->GetUUID();
		root["RenderQueue"] << static_cast<int>(material->GetRenderQueue());

		auto instance = DynamicPointerCast<MaterialInstance>(material);
		bool isInstanced = instance != nullptr;
		root["Instanced"] << (isInstanced ? "true" : "false");

		if (isInstanced)
		{
			if (instance->GetMaterial())
				root["BaseMaterialUUID"] << (uint64_t)instance->GetMaterial()->GetUUID();
			else
				root["BaseMaterialUUID"] << (uint64_t)Constants::InvalidUUID;
		}
		else
		{
			root["Shader"] << (uint64_t)material->GetShader()->GetUUID();
		}

		ryml::NodeRef uniformsNode = root["Uniforms"];
		uniformsNode |= ryml::MAP;

		for (const auto& [name, value] : material->GetUniforms())
		{
			ryml::NodeRef uniformNode = uniformsNode[name.c_str()];
			uniformNode |= ryml::MAP;

			std::visit([&](auto&& arg) {
				using T = std::decay_t<decltype(arg)>;

				if constexpr (std::is_same_v<T, int>) {
					uniformNode["Type"] << "int";
					uniformNode["Value"] << arg;
				}
				else if constexpr (std::is_same_v<T, float>) {
					uniformNode["Type"] << "float";
					uniformNode["Value"] << arg;
				}
				else if constexpr (std::is_same_v<T, Vector2f>) {
					uniformNode["Type"] << "Vector2f";
					Util::SerializeVector2f(uniformNode["Value"], arg);
				}
				else if constexpr (std::is_same_v<T, Vector3f>) {
					uniformNode["Type"] << "Vector3f";
					Util::SerializeVector3f(uniformNode["Value"], arg);
				}
				else if constexpr (std::is_same_v<T, Vector4f>) {
					uniformNode["Type"] << "Vector4f";
					Util::SerializeVector4f(uniformNode["Value"], arg);
				}
				else if constexpr (std::is_same_v<T, Matrix4f>) {
					uniformNode["Type"] << "Matrix4f";
					Util::SerializeMatrix4f(uniformNode["Value"], arg);
				}
				else if constexpr (std::is_same_v<T, SharedPtr<Texture>>) {
					uniformNode["Type"] << "Texture";
					if (arg) uniformNode["Value"] << (uint64_t)arg->GetUUID();
					else uniformNode["Value"] << (uint64_t)Constants::InvalidUUID;
				}
				}, value);
		}

		std::ofstream fout(filepath);
		fout << tree;
		fout.close();
		return true;
	}

	SharedPtr<MaterialBase> MaterialSerializer::Deserialize(UUID uuid, const std::filesystem::path& filepath, AssetManager& assetManager)
	{
		std::ifstream stream(filepath);
		if (!stream.is_open())
		{
			EB_CORE_ERROR("Failed to open material file: {0}", filepath.string());
			return nullptr;
		}

		std::stringstream strStream;
		strStream << stream.rdbuf();
		std::string yamlData = strStream.str();
		stream.close();

		ryml::Tree tree = ryml::parse_in_arena(ryml::to_csubstr(yamlData));
		ryml::NodeRef root = tree.rootref();

		std::string name;
		root["Material"] >> name;

		int renderQueue = 0;
		if (root.has_child("RenderQueue")) root["RenderQueue"] >> renderQueue;

		std::string instancedStr = "false";
		if (root.has_child("Instanced")) {
			root["Instanced"] >> instancedStr;
		}
		else if (root.has_child("BaseMaterial")) {
			instancedStr = "true";
		}

		SharedPtr<MaterialBase> material;

		if (instancedStr == "true")
		{
			uint64_t baseMaterialUUID = Constants::InvalidUUID;
			if (root.has_child("BaseMaterialUUID"))
			{
				root["BaseMaterialUUID"] >> baseMaterialUUID;
			}
			else if (root.has_child("BaseMaterial"))
			{
				std::string baseMatName; root["BaseMaterial"] >> baseMatName;

				auto baseMatAsset = assetManager.GetAsset<Material>(baseMatName);
				if (baseMatAsset)
					baseMaterialUUID = baseMatAsset->GetUUID();
			}

			SharedPtr<Material> baseMaterial = nullptr;
			if (baseMaterialUUID != Constants::InvalidUUID)
				baseMaterial = assetManager.GetAsset<Material>(baseMaterialUUID);

			if (!baseMaterial)
				baseMaterial = assetManager.GetAsset<Material>(Constants::Assets::StandardGeometryMat);

			material = SharedPtr<MaterialInstance>::Create(uuid, name, baseMaterial);
		}
		else
		{
			uint64_t shaderUUID = Constants::InvalidUUID;
			if (root.has_child("Shader"))
				root["Shader"] >> shaderUUID;

			SharedPtr<Shader> shader = nullptr;
			if (shaderUUID != Constants::InvalidUUID)
				shader = assetManager.GetAsset<Shader>(shaderUUID);

			if (!shader)
				shader = assetManager.GetAsset<Shader>(Constants::Assets::StandardGeometryShad);

			material = SharedPtr<Material>::Create(uuid, name, shader, static_cast<RenderQueue>(renderQueue));
		}

		material->SetFilePath(filepath.string());

		// Read Uniforms
		if (root.has_child("Uniforms"))
		{
			ryml::NodeRef uniformsNode = root["Uniforms"];
			for (ryml::NodeRef uniformNode : uniformsNode.children())
			{
				c4::csubstr keyStr = uniformNode.key();
				std::string uniformName(keyStr.str, keyStr.len);

				// 1. If it has a "Type" child, parse using your robust editor format
				if (uniformNode.is_map() && uniformNode.has_child("Type"))
				{
					std::string typeStr; uniformNode["Type"] >> typeStr;
					ryml::NodeRef valNode = uniformNode["Value"];

					if (typeStr == "int") { int val; valNode >> val; material->SetUniform(uniformName, val); }
					else if (typeStr == "float") { float val; valNode >> val; material->SetUniform(uniformName, val); }
					else if (typeStr == "Vector2f") { Vector2f val; Util::DeserializeVector2f(valNode, val); material->SetUniform(uniformName, val); }
					else if (typeStr == "Vector3f") { Vector3f val; Util::DeserializeVector3f(valNode, val); material->SetUniform(uniformName, val); }
					else if (typeStr == "Vector4f") { Vector4f val; Util::DeserializeVector4f(valNode, val); material->SetUniform(uniformName, val); }
					else if (typeStr == "Matrix4f") { Matrix4f val; Util::DeserializeMatrix4f(valNode, val); material->SetUniform(uniformName, val); }
					else if (typeStr == "Texture") {
						uint64_t texUUID; valNode >> texUUID;
						if (texUUID != Constants::InvalidUUID) {
							SharedPtr<Texture> tex = assetManager.GetAsset<Texture>(texUUID);
							if (tex) material->SetUniform(uniformName, tex);
						}
					}
				}
				// 2. Fallback for the simple Cooker format
				else
				{
					if (uniformName == "Albedo") { Vector3f albedo; int i = 0; for (ryml::NodeRef child : uniformNode.children()) child >> albedo[i++]; material->SetUniform(Constants::Uniforms::Albedo, albedo); }
					else if (uniformName == "Emission") { float em; uniformNode >> em; material->SetUniform(Constants::Uniforms::Emission, em); }
					else if (uniformName == "Roughness") { float ro; uniformNode >> ro; material->SetUniform(Constants::Uniforms::Roughness, ro); }
					else if (uniformName == "Metallic") { float me; uniformNode >> me; material->SetUniform(Constants::Uniforms::Metallic, me); }
				}
			}
		}

		// Read Textures (Cooker Format Fallback)
		if (root.has_child("Textures"))
		{
			ryml::NodeRef textures = root["Textures"];
			std::filesystem::path dir = filepath.parent_path();
			auto loadTex = [&](const char* key, const std::string& uniName) {
				if (textures.has_child(key)) {
					std::string tName; textures[key] >> tName;
					std::string tPath = (dir / tName).string();
					auto tex = assetManager.Load<Texture>(tPath);
					if (tex) material->SetUniform(uniName, tex);
				}
				};
			loadTex("AlbedoMap", Constants::Uniforms::AlbedoMap);
			loadTex("NormalMap", Constants::Uniforms::NormalMap);
			loadTex("EmissiveMap", Constants::Uniforms::EmissiveMap);
			loadTex("ORMMap", Constants::Uniforms::MetallicRoughnessMap);
		}

		return material;
	}
}