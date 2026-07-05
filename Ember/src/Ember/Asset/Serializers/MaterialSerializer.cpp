#include "ebpch.h"
#include "MaterialSerializer.h"
#include "AssetManager.h"
#include "Ember/Utils/SerializationUtils.h"

namespace Ember {
	namespace {
		constexpr uint32_t MATERIAL_COOKED_MAGIC = 0x45424D54; // EBMT
		constexpr uint32_t MATERIAL_COOKED_VERSION = 1;

		enum class MaterialUniformType : uint8_t
		{
			Int = 0,
			Float = 1,
			Vector2 = 2,
			Vector3 = 3,
			Vector4 = 4,
			Matrix4 = 5,
			Texture = 6
		};

		template<typename T>
		void WriteRaw(std::ofstream& stream, const T& value)
		{
			stream.write(reinterpret_cast<const char*>(&value), sizeof(T));
		}

		template<typename T>
		bool ReadRaw(std::ifstream& stream, T& value)
		{
			stream.read(reinterpret_cast<char*>(&value), sizeof(T));
			return stream.good();
		}

		void WriteString(std::ofstream& stream, const std::string& value)
		{
			uint16_t len = static_cast<uint16_t>(value.size());
			WriteRaw(stream, len);
			if (len > 0)
				stream.write(value.data(), len);
		}

		bool ReadString(std::ifstream& stream, std::string& value)
		{
			uint16_t len = 0;
			if (!ReadRaw(stream, len))
				return false;

			value.resize(len);
			if (len > 0)
				stream.read(value.data(), len);

			return stream.good();
		}

		std::filesystem::path GetCookedPath(const std::filesystem::path& filepath)
		{
			auto cookedPath = filepath;
			cookedPath.replace_extension(".bin");
			return cookedPath;
		}
	}

	bool MaterialSerializer::SerializeSource(const std::filesystem::path& filepath, const SharedPtr<MaterialBase>& material)
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
		else if (material->GetShader())
			root["Shader"] << (uint64_t)material->GetShader()->GetUUID();
		else 
			root["Shader"] << (uint64_t)Constants::InvalidUUID;

		ryml::NodeRef uniformsNode = root["Uniforms"];
		uniformsNode |= ryml::MAP;

		for (const auto& [name, value] : material->GetUniforms())
		{
			ryml::NodeRef uniformNode = uniformsNode[name.c_str()];
			uniformNode |= ryml::MAP;

			// Use std::visit to serialize each uniform based on its variant type
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
				else if constexpr (std::is_same_v<T, SharedPtr<Texture2D>>) {
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

	SharedPtr<MaterialBase> MaterialSerializer::DeserializeSource(UUID uuid, const std::filesystem::path& filepath, AssetManager& assetManager)
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
		if (root.has_child("RenderQueue"))
			root["RenderQueue"] >> renderQueue;

		std::string instancedStr = "false";
		if (root.has_child("Instanced"))
			root["Instanced"] >> instancedStr;
		else if (root.has_child("BaseMaterial"))
			instancedStr = "true";

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

			// Fall back to default geometry material if the base material can't be found
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

				// Editor format stores typed key-value pairs; cooker format uses simple names
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
							SharedPtr<Texture2D> tex = assetManager.GetAsset<Texture2D>(texUUID);
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
					auto tex = assetManager.Load<Texture2D>(tPath);
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

	bool MaterialSerializer::SerializeCooked(const std::filesystem::path& filepath, const SharedPtr<MaterialBase>& material)
	{
		if (!material)
			return false;

		auto outputPath = GetCookedPath(filepath);
		std::ofstream stream(outputPath, std::ios::binary | std::ios::trunc);
		if (!stream.is_open())
			return false;

		WriteRaw(stream, MATERIAL_COOKED_MAGIC);
		WriteRaw(stream, MATERIAL_COOKED_VERSION);

		WriteString(stream, material->GetName());

		int32_t renderQueue = static_cast<int32_t>(material->GetRenderQueue());
		WriteRaw(stream, renderQueue);

		auto instance = DynamicPointerCast<MaterialInstance>(material);
		uint8_t isInstanced = instance ? 1 : 0;
		WriteRaw(stream, isInstanced);

		uint64_t referencedUUID = Constants::InvalidUUID;
		if (instance)
			referencedUUID = instance->GetMaterial() ? static_cast<uint64_t>(instance->GetMaterial()->GetUUID()) : static_cast<uint64_t>(Constants::InvalidUUID);
		else
			referencedUUID = material->GetShader() ? static_cast<uint64_t>(material->GetShader()->GetUUID()) : static_cast<uint64_t>(Constants::InvalidUUID);

		WriteRaw(stream, referencedUUID);

		const auto& uniforms = material->GetUniforms();
		uint32_t uniformCount = static_cast<uint32_t>(uniforms.size());
		WriteRaw(stream, uniformCount);

		for (const auto& [name, value] : uniforms)
		{
			WriteString(stream, name);

			std::visit([&](auto&& arg) {
				using T = std::decay_t<decltype(arg)>;
				if constexpr (std::is_same_v<T, int>)
				{
					uint8_t type = static_cast<uint8_t>(MaterialUniformType::Int);
					WriteRaw(stream, type);
					WriteRaw(stream, arg);
				}
				else if constexpr (std::is_same_v<T, float>)
				{
					uint8_t type = static_cast<uint8_t>(MaterialUniformType::Float);
					WriteRaw(stream, type);
					WriteRaw(stream, arg);
				}
				else if constexpr (std::is_same_v<T, Vector2f>)
				{
					uint8_t type = static_cast<uint8_t>(MaterialUniformType::Vector2);
					WriteRaw(stream, type);
					WriteRaw(stream, arg);
				}
				else if constexpr (std::is_same_v<T, Vector3f>)
				{
					uint8_t type = static_cast<uint8_t>(MaterialUniformType::Vector3);
					WriteRaw(stream, type);
					WriteRaw(stream, arg);
				}
				else if constexpr (std::is_same_v<T, Vector4f>)
				{
					uint8_t type = static_cast<uint8_t>(MaterialUniformType::Vector4);
					WriteRaw(stream, type);
					WriteRaw(stream, arg);
				}
				else if constexpr (std::is_same_v<T, Matrix4f>)
				{
					uint8_t type = static_cast<uint8_t>(MaterialUniformType::Matrix4);
					WriteRaw(stream, type);
					WriteRaw(stream, arg);
				}
				else if constexpr (std::is_same_v<T, SharedPtr<Texture2D>>)
				{
					uint8_t type = static_cast<uint8_t>(MaterialUniformType::Texture);
					WriteRaw(stream, type);
					uint64_t textureUUID = arg ? static_cast<uint64_t>(arg->GetUUID()) : static_cast<uint64_t>(Constants::InvalidUUID);
					WriteRaw(stream, textureUUID);
				}
			}, value);
		}

		return true;
	}

	SharedPtr<MaterialBase> MaterialSerializer::DeserializeCooked(UUID uuid, const std::filesystem::path& filepath, AssetManager& assetManager)
	{
		std::ifstream stream(filepath, std::ios::binary);
		if (!stream.is_open())
			return nullptr;

		uint32_t magic = 0;
		uint32_t version = 0;
		if (!ReadRaw(stream, magic) || !ReadRaw(stream, version))
			return nullptr;

		if (magic != MATERIAL_COOKED_MAGIC || version > MATERIAL_COOKED_VERSION)
			return nullptr;

		std::string name;
		if (!ReadString(stream, name))
			return nullptr;

		int32_t renderQueue = 0;
		uint8_t isInstanced = 0;
		uint64_t referencedUUID = Constants::InvalidUUID;
		if (!ReadRaw(stream, renderQueue) || !ReadRaw(stream, isInstanced) || !ReadRaw(stream, referencedUUID))
			return nullptr;

		SharedPtr<MaterialBase> material;
		if (isInstanced != 0)
		{
			auto base = referencedUUID != Constants::InvalidUUID ? assetManager.GetAsset<Material>(UUID(referencedUUID)) : nullptr;
			if (!base)
				base = assetManager.GetAsset<Material>(Constants::Assets::StandardGeometryMat);
			material = SharedPtr<MaterialInstance>::Create(uuid, name, base);
		}
		else
		{
			auto shader = referencedUUID != Constants::InvalidUUID ? assetManager.GetAsset<Shader>(UUID(referencedUUID)) : nullptr;
			if (!shader)
				shader = assetManager.GetAsset<Shader>(Constants::Assets::StandardGeometryShad);
			material = SharedPtr<Material>::Create(uuid, name, shader, static_cast<RenderQueue>(renderQueue));
		}

		uint32_t uniformCount = 0;
		if (!ReadRaw(stream, uniformCount))
			return material;

		for (uint32_t i = 0; i < uniformCount; i++)
		{
			std::string uniformName;
			uint8_t rawType = 0;
			if (!ReadString(stream, uniformName) || !ReadRaw(stream, rawType))
				return material;

			auto type = static_cast<MaterialUniformType>(rawType);
			switch (type)
			{
			case MaterialUniformType::Int:
			{
				int value;
				if (ReadRaw(stream, value)) material->SetUniform(uniformName, value);
				break;
			}
			case MaterialUniformType::Float:
			{
				float value;
				if (ReadRaw(stream, value)) material->SetUniform(uniformName, value);
				break;
			}
			case MaterialUniformType::Vector2:
			{
				Vector2f value;
				if (ReadRaw(stream, value)) material->SetUniform(uniformName, value);
				break;
			}
			case MaterialUniformType::Vector3:
			{
				Vector3f value;
				if (ReadRaw(stream, value)) material->SetUniform(uniformName, value);
				break;
			}
			case MaterialUniformType::Vector4:
			{
				Vector4f value;
				if (ReadRaw(stream, value)) material->SetUniform(uniformName, value);
				break;
			}
			case MaterialUniformType::Matrix4:
			{
				Matrix4f value;
				if (ReadRaw(stream, value)) material->SetUniform(uniformName, value);
				break;
			}
			case MaterialUniformType::Texture:
			{
				uint64_t textureUUID = Constants::InvalidUUID;
				if (ReadRaw(stream, textureUUID) && textureUUID != Constants::InvalidUUID)
				{
					auto tex = assetManager.GetAsset<Texture2D>(UUID(textureUUID));
					if (tex) material->SetUniform(uniformName, tex);
				}
				break;
			}
			default:
				break;
			}
		}

		material->SetFilePath(filepath.string());
		return material;
	}

	bool MaterialSerializer::Serialize(const std::filesystem::path& filepath, const SharedPtr<MaterialBase>& material)
	{
		return SerializeSource(filepath, material);
	}

	SharedPtr<MaterialBase> MaterialSerializer::Deserialize(UUID uuid, const std::filesystem::path& filepath, AssetManager& assetManager)
	{
		switch (AssetSerializationMode::GetRuntimeLoadTier())
		{
		case RuntimeAssetLoadTier::ForceSourceYaml:
			return DeserializeSource(uuid, filepath, assetManager);
		case RuntimeAssetLoadTier::ForceCookedBinary:
			return DeserializeCooked(uuid, GetCookedPath(filepath), assetManager);
		case RuntimeAssetLoadTier::Auto:
		default:
			if (filepath.extension() == ".bin")
				return DeserializeCooked(uuid, filepath, assetManager);
			return DeserializeSource(uuid, filepath, assetManager);
		}
	}
}