#pragma once

#include "Ember/Core/Core.h"
#include "Ember/Render/Material.h"
#include "AssetSerializationMode.h"

#include <ryml.hpp>
#include <ryml_std.hpp>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace Ember {

	class AssetManager;

	class MaterialSerializer
	{
	public:
		static bool SerializeSource(const std::filesystem::path& filepath, const SharedPtr<MaterialBase>& material);
		static SharedPtr<MaterialBase> DeserializeSource(UUID uuid, const std::filesystem::path& filepath, AssetManager& assetManager);

		static bool SerializeCooked(const std::filesystem::path& filepath, const SharedPtr<MaterialBase>& material);
		static SharedPtr<MaterialBase> DeserializeCooked(UUID uuid, const std::filesystem::path& filepath, AssetManager& assetManager);

		static bool Serialize(const std::filesystem::path& filepath, const SharedPtr<MaterialBase>& material);
		static SharedPtr<MaterialBase> Deserialize(UUID uuid, const std::filesystem::path& filepath, AssetManager& assetManager);
	};

}