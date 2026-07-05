#pragma once

#include "PhysicsMaterial.h"
#include "AssetSerializationMode.h"

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
		static bool SerializeSource(const std::filesystem::path& filepath, const SharedPtr<PhysicsMaterial>& material);
		static SharedPtr<PhysicsMaterial> DeserializeSource(UUID uuid, const std::filesystem::path& filepath);

		static bool SerializeCooked(const std::filesystem::path& filepath, const SharedPtr<PhysicsMaterial>& material);
		static SharedPtr<PhysicsMaterial> DeserializeCooked(UUID uuid, const std::filesystem::path& filepath);

		static bool Serialize(const std::filesystem::path& filepath, const SharedPtr<PhysicsMaterial>& material);
		static SharedPtr<PhysicsMaterial> Deserialize(UUID uuid, const std::filesystem::path& filepath);
	};

}