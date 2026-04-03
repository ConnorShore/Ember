#pragma once

#include "UUID.h"
#include "Model.h"
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
		static SharedPtr<Model> Deserialize(UUID uuid, const std::filesystem::path& filepath, AssetManager& assetManager);

	private:
		static void DeserializeNode(ryml::NodeRef yamlNode, ModelNode& modelNode, AssetManager& assetManager, std::vector<SharedPtr<MaterialBase>>& materials, std::unordered_map<UUID, unsigned int>& materialIndexMap);
	};

}