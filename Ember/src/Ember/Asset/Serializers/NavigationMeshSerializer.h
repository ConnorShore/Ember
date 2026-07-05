#pragma once


#include "Ember/Core/Core.h"
#include "Ember/Asset/NavigationMeshData.h"

#include <fstream>
#include <sstream>
#include <filesystem>

namespace Ember {

	class NavigationMeshSerializer
	{
	public:
		static bool SerializeSource(const std::filesystem::path& filepath, const SharedPtr<NavigationMeshData>& navMesh);
		static SharedPtr<NavigationMeshData> DeserializeSource(UUID uuid, const std::filesystem::path& filepath);

		static bool SerializeCooked(const std::filesystem::path& filepath, const SharedPtr<NavigationMeshData>& navMesh);
		static SharedPtr<NavigationMeshData> DeserializeCooked(UUID uuid, const std::filesystem::path& filepath);

		static bool Serialize(const std::filesystem::path& filepath, const SharedPtr<NavigationMeshData>& mesh);
		static SharedPtr<NavigationMeshData> Deserialize(UUID uuid, const std::filesystem::path& filepath);
	};

}