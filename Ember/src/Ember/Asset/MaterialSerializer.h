#pragma once

#include "Ember/Core/Core.h"
#include "Ember/Render/Material.h"

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
		static bool Serialize(const std::filesystem::path& filepath, const SharedPtr<MaterialBase>& material);
		static SharedPtr<MaterialBase> Deserialize(UUID uuid, const std::filesystem::path& filepath, AssetManager& assetManager);
	};

}