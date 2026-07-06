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

	private:
		struct BuildPipelineState;

		static bool ExtractStaticMeshGeometry(BuildPipelineState& state, BuildResult& result);

		static bool BuildRecastPipeline(BuildPipelineState& state, BuildResult& result);
		static bool InitializeConfigAndBounds(BuildPipelineState& state, BuildResult& result);
		static bool PrepareHeightfieldInput(BuildPipelineState& state, BuildResult& result);
		static bool RasterizeInputMesh(BuildPipelineState& state, BuildResult& result);
		static bool FilterWalkableSurfaces(BuildPipelineState& state, BuildResult& result);
		static bool PartitionWalkableSurface(BuildPipelineState& state, BuildResult& result);
		static bool TraceAndSimplifyContours(BuildPipelineState& state, BuildResult& result);
		static bool TriangulateContoursToPolyMesh(BuildPipelineState& state, BuildResult& result);
		static bool BuildPolyMeshDetail(BuildPipelineState& state, BuildResult& result);

		static bool CreateDetourNavMesh(BuildPipelineState& state, BuildResult& result);
	};

}