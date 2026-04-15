#pragma once

#include "PhysicsMaterial.h"

#include "Ember/Core/Core.h"

#include <ryml.hpp>
#include <ryml_std.hpp>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace Ember {

	class AssetManager;

	class PhysicsMaterialSerializer
	{
	public:
		static bool Serialize(const std::filesystem::path& filepath, const SharedPtr<PhysicsMaterial>& material);
		static SharedPtr<PhysicsMaterial> Deserialize(UUID uuid, const std::filesystem::path& filepath);
	};

}