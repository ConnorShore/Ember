#pragma once

#include "Ember/Asset/UUID.h"
#include "Ember/Asset/Model.h"
#include "AssetSerializationMode.h"
#include "Ember/Core/Core.h"

#include <ryml.hpp>
#include <ryml_std.hpp>

#include <fstream>
#include <sstream>
#include <filesystem>
#include <unordered_map>

namespace Ember {

	class AssetManager;

	class ModelSerializer
	{
	public:
		static bool SerializeSource(const std::filesystem::path& filepath, const SharedPtr<Model>& model);
		static SharedPtr<Model> DeserializeSource(UUID uuid, const std::filesystem::path& filepath, AssetManager& assetManager);

		static bool SerializeCooked(const std::filesystem::path& filepath, const SharedPtr<Model>& model);
		static SharedPtr<Model> DeserializeCooked(UUID uuid, const std::filesystem::path& filepath, AssetManager& assetManager);

		static bool Serialize(const std::filesystem::path& filepath, const SharedPtr<Model>& model);
		static SharedPtr<Model> Deserialize(UUID uuid, const std::filesystem::path& filepath, AssetManager& assetManager);

	private:
		static void SerializeNode(ryml::NodeRef yamlNode, const ModelNode& modelNode, const std::vector<SharedPtr<MaterialBase>>& materials);
		static void DeserializeNode(ryml::NodeRef yamlNode, ModelNode& modelNode, AssetManager& assetManager, std::vector<SharedPtr<MaterialBase>>& materials, std::unordered_map<UUID, uint32_t>& materialIndexMap);
	};

}