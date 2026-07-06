#pragma once

#include "NavigationMeshBakeSettings.h"

#include <DetourNavMesh.h>

#include <string>
#include <vector>

namespace Ember {

	class Scene;
	class Entity;

	class NavigationMeshBuilder
	{
	public:
		struct BuildResult
		{
			bool Success = false;
			std::string Error;
			std::vector<uint8_t> RawDataBlob;
			dtNavMesh* RuntimeNavMesh = nullptr;
		};

		static BuildResult BuildNavigationMesh(const SharedPtr<Scene>& scene, Entity navMeshEntity, const NavigationMeshBakeSettings& settings);
	};

}