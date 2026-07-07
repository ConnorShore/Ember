#include "ebpch.h"
#include "NavigationMeshBuilder.h"

#include "Ember/Core/Application.h"
#include "Ember/Render/Mesh.h"
#include "Ember/Scene/Scene.h"

#include <Recast.h>
#include <DetourNavMeshBuilder.h>
#include <DetourNavMeshQuery.h>

#include <algorithm>
#include <limits>

namespace Ember {
	namespace {
		struct HeightfieldResource
		{
			rcHeightfield* Ptr = rcAllocHeightfield();
			~HeightfieldResource()
			{
				if (Ptr)
					rcFreeHeightField(Ptr);
			}
		};

		struct CompactHeightfieldResource
		{
			rcCompactHeightfield* Ptr = rcAllocCompactHeightfield();
			~CompactHeightfieldResource()
			{
				if (Ptr)
					rcFreeCompactHeightfield(Ptr);
			}
		};

		struct ContourSetResource
		{
			rcContourSet* Ptr = rcAllocContourSet();
			~ContourSetResource()
			{
				if (Ptr)
					rcFreeContourSet(Ptr);
			}
		};

		struct PolyMeshResource
		{
			rcPolyMesh* Ptr = rcAllocPolyMesh();
			~PolyMeshResource()
			{
				if (Ptr)
					rcFreePolyMesh(Ptr);
			}
		};

		struct PolyMeshDetailResource
		{
			rcPolyMeshDetail* Ptr = rcAllocPolyMeshDetail();
			~PolyMeshDetailResource()
			{
				if (Ptr)
					rcFreePolyMeshDetail(Ptr);
			}
		};

		struct TriAreasResource
		{
			explicit TriAreasResource(int count)
				: Count(count), Ptr(count > 0 ? new unsigned char[count] : nullptr)
			{
			}

			~TriAreasResource()
			{
				delete[] Ptr;
				Ptr = nullptr;
			}

			int Count = 0;
			unsigned char* Ptr = nullptr;
		};

		struct AABB
		{
			Vector3f Min = Vector3f(0.0f);
			Vector3f Max = Vector3f(0.0f);
		};

		static AABB BuildNavBoundsAABB(Entity navMeshEntity)
		{
			auto& transform = navMeshEntity.GetComponent<TransformComponent>();
			auto& navComp = navMeshEntity.GetComponent<NavigationMeshComponent>();

			const Matrix4f& world = transform.GetWorldTransform();
			const Vector3f centerWorld = world * navComp.BoundsCenterOffset;

			// Use world basis lengths so bounds honor parent scaling.
			const Vector3f worldScale(
				glm::length(Vector3f(world[0])),
				glm::length(Vector3f(world[1])),
				glm::length(Vector3f(world[2]))
			);

			const Vector3f halfExtents = Vector3f(
				navComp.BoundsSize.x * worldScale.x,
				navComp.BoundsSize.y * worldScale.y,
				navComp.BoundsSize.z * worldScale.z
			) * 0.5f;

			AABB bounds;
			bounds.Min = centerWorld - halfExtents;
			bounds.Max = centerWorld + halfExtents;
			return bounds;
		}

		static AABB BuildWorldAABBFromLocalBounds(const Matrix4f& worldTransform, const Vector3f& localMin, const Vector3f& localMax)
		{
			Vector3f corners[8] = {
				{ localMin.x, localMin.y, localMin.z },
				{ localMax.x, localMin.y, localMin.z },
				{ localMin.x, localMax.y, localMin.z },
				{ localMax.x, localMax.y, localMin.z },
				{ localMin.x, localMin.y, localMax.z },
				{ localMax.x, localMin.y, localMax.z },
				{ localMin.x, localMax.y, localMax.z },
				{ localMax.x, localMax.y, localMax.z }
			};

			AABB worldAabb;
			worldAabb.Min = Vector3f(std::numeric_limits<float>::max());
			worldAabb.Max = Vector3f(std::numeric_limits<float>::lowest());

			for (const auto& corner : corners)
			{
				const Vector3f p = worldTransform * corner;
				worldAabb.Min = Math::Min(worldAabb.Min, p);
				worldAabb.Max = Math::Max(worldAabb.Max, p);
			}

			return worldAabb;
		}

		static bool AABBOverlaps(const AABB& a, const AABB& b)
		{
			if (a.Max.x < b.Min.x || a.Min.x > b.Max.x) return false;
			if (a.Max.y < b.Min.y || a.Min.y > b.Max.y) return false;
			if (a.Max.z < b.Min.z || a.Min.z > b.Max.z) return false;
			return true;
		}
	}

	struct NavigationMeshBuilder::BuildPipelineState
	{
		BuildPipelineState(const SharedPtr<Scene>& scene, Entity navMeshEntity, const NavigationMeshBakeSettings& settings)
			: SceneHandle(scene), NavMeshEntity(navMeshEntity), Settings(settings)
		{
			Vertices.reserve(4096);
			Triangles.reserve(4096);
		}

		const SharedPtr<Scene>& SceneHandle;
		Entity NavMeshEntity;
		const NavigationMeshBakeSettings& Settings;

		AABB NavBounds;
		std::vector<float> Vertices;
		std::vector<int> Triangles;

		int NumVerts = 0;
		int NumTris = 0;
		const float* RecastVerts = nullptr;
		const int* RecastTris = nullptr;

		ScopedPtr<rcContext> BuildContext;
		rcConfig Config{};

		ScopedPtr<HeightfieldResource> Heightfield;
		ScopedPtr<TriAreasResource> TriAreas;
		ScopedPtr<CompactHeightfieldResource> CompactHeightfield;
		ScopedPtr<ContourSetResource> ContourSet;
		ScopedPtr<PolyMeshResource> PolyMesh;
		ScopedPtr<PolyMeshDetailResource> DetailMesh;
	};

	NavigationMeshBuilder::BuildResult NavigationMeshBuilder::BuildNavigationMesh(const SharedPtr<Scene>& scene, Entity navMeshEntity, const NavigationMeshBakeSettings& settings)
	{
		BuildResult result;
		if (!scene)
		{
			result.Error = "Navigation mesh bake failed: Scene is null.";
			return result;
		}

		if (!navMeshEntity)
		{
			result.Error = "Navigation mesh bake failed: Nav mesh entity is invalid.";
			return result;
		}

		BuildPipelineState state(scene, navMeshEntity, settings);

		// TODO: Have ability to filter either rendered geometry or physics colliders only (using rendered geometry for now).
		EB_CORE_ASSERT(navMeshEntity.ContainsComponent<NavigationMeshComponent>(), "Entity must contain navigation mesh component");
		state.NavBounds = BuildNavBoundsAABB(navMeshEntity);

		if (!ExtractStaticMeshGeometry(state, result))
			return result;

		if (!BuildRecastPipeline(state, result))
			return result;

		if (!CreateDetourNavMesh(state, result))
			return result;

		return result;
	}

	bool NavigationMeshBuilder::ExtractStaticMeshGeometry(BuildPipelineState& state, BuildResult& result)
	{
		// 1. Extract all static mesh geometry from the scene that overlaps the nav bounds.
		auto& assetManager = Application::Instance().GetAssetManager();

		auto& registry = state.SceneHandle->GetRegistry();
		for (EntityID staticMesh : registry.ActiveQuery<StaticMeshComponent, TransformComponent>())
		{
			auto [meshComp, transform] = registry.GetComponents<StaticMeshComponent, TransformComponent>(staticMesh);
			if (meshComp.MeshHandle == Constants::InvalidUUID)
				continue;

			auto mesh = assetManager.GetAsset<Mesh>(meshComp.MeshHandle);
			if (!mesh)
				continue;

			const Matrix4f& world = transform.GetWorldTransform();
			const AABB meshWorldAABB = BuildWorldAABBFromLocalBounds(world, mesh->GetMinBounds(), mesh->GetMaxBounds());
			if (!AABBOverlaps(meshWorldAABB, state.NavBounds))
				continue;

			auto localVerts = mesh->GetVertexPositions();
			auto localTris = mesh->GetTriangles();

			if (localVerts.size() < 3 || localTris.size() < 3)
				continue;

			const int baseVertex = static_cast<int>(state.Vertices.size() / 3);

			for (size_t i = 0; i + 2 < localVerts.size(); i += 3)
			{
				const Vector3f localPos(localVerts[i], localVerts[i + 1], localVerts[i + 2]);
				const Vector3f worldPos = world * localPos;

				state.Vertices.push_back(worldPos.x);
				state.Vertices.push_back(worldPos.y);
				state.Vertices.push_back(worldPos.z);
			}

			for (uint32_t idx : localTris)
			{
				state.Triangles.push_back(baseVertex + static_cast<int>(idx));
			}
		}

		state.NumVerts = static_cast<int>(state.Vertices.size() / 3);
		state.NumTris = static_cast<int>(state.Triangles.size() / 3);
		if (state.NumVerts == 0 || state.NumTris == 0)
		{
			result.Error = "Navigation mesh bake failed: no static mesh geometry overlaps the nav bounds.";
			return false;
		}

		state.RecastVerts = state.Vertices.data();
		state.RecastTris = state.Triangles.data();
		return true;
	}

	bool NavigationMeshBuilder::BuildRecastPipeline(BuildPipelineState& state, BuildResult& result)
	{
		// 2) Build rcConfig from settings and run Recast pipeline.
		if (!InitializeConfigAndBounds(state, result))
			return false;

		if (!PrepareHeightfieldInput(state, result))
			return false;

		if (!RasterizeInputMesh(state, result))
			return false;

		if (!FilterWalkableSurfaces(state, result))
			return false;

		if (!PartitionWalkableSurface(state, result))
			return false;

		if (!TraceAndSimplifyContours(state, result))
			return false;

		if (!TriangulateContoursToPolyMesh(state, result))
			return false;

		if (!BuildPolyMeshDetail(state, result))
			return false;

		return true;
	}

	bool NavigationMeshBuilder::InitializeConfigAndBounds(BuildPipelineState& state, BuildResult& result)
	{
		state.BuildContext = ScopedPtr<rcContext>::Create(false);

		memset(&state.Config, 0, sizeof(state.Config));
		state.Config.cs = state.Settings.CellSize;
		state.Config.ch = state.Settings.CellHeight;
		state.Config.walkableSlopeAngle = state.Settings.AgentMaxSlope;
		state.Config.walkableHeight = static_cast<int>(ceilf(state.Settings.AgentHeight / state.Config.ch));
		state.Config.walkableClimb = static_cast<int>(floorf(state.Settings.AgentMaxClimb / state.Config.ch));
		state.Config.walkableRadius = static_cast<int>(ceilf(state.Settings.AgentRadius / state.Config.cs));

		// TODO: Add config settings for rest of props once we get them from user, for now use default values
		state.Config.maxEdgeLen = 12; // Default value
		state.Config.maxSimplificationError = 1.3f; // Default value
		state.Config.minRegionArea = 8; // Default value
		state.Config.mergeRegionArea = 20; // Default value
		state.Config.maxVertsPerPoly = 6; // Default value
		state.Config.detailSampleDist = 6.0f; // Default value
		state.Config.detailSampleMaxError = 1.0f; // Default value
		//config.maxEdgeLen = static_cast<int>(edgeMaxLen / cellSize);
		//config.maxSimplificationError = edgeMaxError;
		//config.minRegionArea = static_cast<int>(rcSqr(regionMinSize));      // Note: area = size*size
		//config.mergeRegionArea = static_cast<int>(rcSqr(regionMergeSize));  // Note: area = size*size
		//config.maxVertsPerPoly = vertsPerPoly;
		//config.detailSampleDist = detailSampleDist < 0.9f ? 0 : cellSize * detailSampleDist;
		//config.detailSampleMaxError = cellHeight * detailSampleMaxError;

		// Set the area where the navigation will be built.
		// Here the bounds of the input mesh are used, but the
		// area could be specified by a user defined box, etc.
		float boundsMin[3] = {
			std::numeric_limits<float>::max(),
			std::numeric_limits<float>::max(),
			std::numeric_limits<float>::max()
		};
		float boundsMax[3] = {
			std::numeric_limits<float>::lowest(),
			std::numeric_limits<float>::lowest(),
			std::numeric_limits<float>::lowest()
		};

		for (size_t i = 0; i + 2 < state.Vertices.size(); i += 3)
		{
			boundsMin[0] = std::min(boundsMin[0], state.Vertices[i + 0]);
			boundsMin[1] = std::min(boundsMin[1], state.Vertices[i + 1]);
			boundsMin[2] = std::min(boundsMin[2], state.Vertices[i + 2]);

			boundsMax[0] = std::max(boundsMax[0], state.Vertices[i + 0]);
			boundsMax[1] = std::max(boundsMax[1], state.Vertices[i + 1]);
			boundsMax[2] = std::max(boundsMax[2], state.Vertices[i + 2]);
		}

		rcVcopy(state.Config.bmin, boundsMin);
		rcVcopy(state.Config.bmax, boundsMax);
		rcCalcGridSize(state.Config.bmin, state.Config.bmax, state.Config.cs, &state.Config.width, &state.Config.height);

		return true;
	}

	bool NavigationMeshBuilder::PrepareHeightfieldInput(BuildPipelineState& state, BuildResult& result)
	{
		// Rasterize input meshes.
		// Allocate voxel heightfield where we will store our rasterized input data.
		state.Heightfield = ScopedPtr<HeightfieldResource>::Create();
		if (!state.Heightfield || !state.Heightfield->Ptr)
		{
			result.Error = "Navigation mesh bake failed: Out of memory 'heightfield'.";
			return false;
		}

		if (!rcCreateHeightfield(
			state.BuildContext.Ptr(),
			*state.Heightfield->Ptr,
			state.Config.width,
			state.Config.height,
			state.Config.bmin,
			state.Config.bmax,
			state.Config.cs,
			state.Config.ch))
		{
			result.Error = "Navigation mesh bake failed: Could not create solid heightfield.";
			return false;
		}

		// Allocate array that can hold triangle area types.
		// This is used to store terrain type information and to mark
		// triangles as unwalkable.
		// If you have multiple meshes you need to process, allocate
		// an array which can hold the max number of triangles you need to process.
		state.TriAreas = ScopedPtr<TriAreasResource>::Create(state.NumTris);
		if (!state.TriAreas || !state.TriAreas->Ptr)
		{
			result.Error = "Navigation mesh bake failed: Out of memory 'triAreas'.";
			return false;
		}
		memset(state.TriAreas->Ptr, 0, static_cast<size_t>(state.TriAreas->Count) * sizeof(unsigned char));

		// Record which triangles in the input mesh are walkable.
		// This information is recorded in triAreas
		rcMarkWalkableTriangles(state.BuildContext.Ptr(), state.Config.walkableSlopeAngle, state.RecastVerts, state.NumVerts, state.RecastTris, state.NumTris, state.TriAreas->Ptr);
		return true;
	}

	bool NavigationMeshBuilder::RasterizeInputMesh(BuildPipelineState& state, BuildResult& result)
	{
		// 2.1: Rasterize the input mesh
		// If your have multiple meshes, you can transform them, calculate the
		// terrain type for each mesh and rasterize them here.
		if (!rcRasterizeTriangles(state.BuildContext.Ptr(), state.RecastVerts, state.NumVerts, state.RecastTris, state.TriAreas->Ptr, state.NumTris, *state.Heightfield->Ptr, state.Config.walkableClimb))
		{
			result.Error = "Navigation mesh bake failed: Could not rasterize triangles.";
			return false;
		}
		return true;
	}

	bool NavigationMeshBuilder::FilterWalkableSurfaces(BuildPipelineState& state, BuildResult& result)
	{
		(void)result;

		// 2.2: Filter walkable surfaces.
		// Once all geometry is rasterized, we do initial pass of filtering to
		// remove unwanted overhangs caused by the conservative rasterization
		// as well as spans where the character cannot possibly stand.
		// TODO: Make these configurable
		bool filterLowHangingObstacles = true;
		bool filterLedgeSpans = true;
		bool filterWalkableLowHeightSpans = true;

		if (filterLowHangingObstacles)
		{
			rcFilterLowHangingWalkableObstacles(state.BuildContext.Ptr(), state.Config.walkableClimb, *state.Heightfield->Ptr);
		}
		if (filterLedgeSpans)
		{
			rcFilterLedgeSpans(state.BuildContext.Ptr(), state.Config.walkableHeight, state.Config.walkableClimb, *state.Heightfield->Ptr);
		}
		if (filterWalkableLowHeightSpans)
		{
			rcFilterWalkableLowHeightSpans(state.BuildContext.Ptr(), state.Config.walkableHeight, *state.Heightfield->Ptr);
		}

		return true;
	}

	bool NavigationMeshBuilder::PartitionWalkableSurface(BuildPipelineState& state, BuildResult& result)
	{
		// 2.3: Partition walkable surface into simple regions.
		// Compact the heightfield so that it is faster to work with.
		// This will result more cache coherent data.  This step will also
		// generate neighbor connection information between walkable cells.
		state.CompactHeightfield = ScopedPtr<CompactHeightfieldResource>::Create();
		if (!state.CompactHeightfield || !state.CompactHeightfield->Ptr)
		{
			result.Error = "Navigation mesh bake failed: Out of memory 'compactHeightfield'.";
			return false;
		}
		if (!rcBuildCompactHeightfield(
			state.BuildContext.Ptr(),
			state.Config.walkableHeight,
			state.Config.walkableClimb,
			*state.Heightfield->Ptr,
			*state.CompactHeightfield->Ptr))
		{
			result.Error = "Navigation mesh bake failed: Could not build compact data.";
			return false;
		}

		// Erode the walkable area by agent radius.
		// This allows us to path an agent through the navmesh as if it was a single point
		if (!rcErodeWalkableArea(state.BuildContext.Ptr(), state.Config.walkableRadius, *state.CompactHeightfield->Ptr))
		{
			result.Error = "Navigation mesh bake failed: Could not erode walkable area.";
			return false;
		}

		// (Optional) Marks the surface type of voxels in an area defined by a convex volume.
		// Useful to mark areas of differing cost.
		//static constexpr int MAX_CONVEXVOL_PTS = 12;
		//struct ConvexVolume
		//{
		//	float verts[MAX_CONVEXVOL_PTS * 3] = {};
		//	int nverts = 0;
		//	float hmin = 0.0f;
		//	float hmax = 0.0f;
		//	int area = 0;
		//};
		//for (ConvexVolume& vol : inputGeometry->convexVolumes)
		//{
		//	rcMarkConvexPolyArea(
		//		buildContext,
		//		vol.verts,
		//		vol.nverts,
		//		vol.hmin,
		//		vol.hmax,
		//		(unsigned char)vol.area,
		//		*compactHeightfield);
		//}

		// Partition the heightfield into contiguous regions that will each be
		// triangulated into navigation polygons.
		//
		// There are 3 partitioning methods, each with their own pros and cons:
		// 1) Watershed partitioning
		//   - the classic Recast partitioning
		//   - creates the nicest tessellation
		//   - usually slowest
		//   - the are some corner cases where this method creates holes and
		//     overlaps in the resulting region data.
		//      - holes may appear when a small obstacle is close to a large open
		//        area.  This will not cause triangulation to fail.
		//      - overlaps may occur if you have narrow spiral corridors
		//        e.g. spiral stairs.  This will cause triangulation to fail.
		//   * Generally the best choice if you are precompute the navmesh and/or
		//     there are large open areas in the input geometry.
		// 2) Monotone partitioning
		//   - fastest
		//   - guaranteed to partition the heightfield into regions without holes
		//     or overlaps
		//   - Can create long, thin polygons which sometimes cause paths with detours
		//   * Use this if you want fast navmesh generation
		// 3) Layer partitioning
		//   - quite fast
		//   - partitions the heighfield into non-overlapping regions
		//   - relies on the triangulation code to cope with holes, which makes
		//     this slower than monotone partitioning
		//   - produces better triangles than monotone partitioning
		//   - does not have the corner cases of watershed partitioning
		//   - can be slow and create a slightly ugly tessellation (still better
		//     than monotone) if you have large open areas with small obstacles.
		//     This is less of a problem if you use a tiled navmesh.
		//   * A good choice for a tiled navmesh with small to medium-sized tiles

		//if (partitionType == SamplePartitionType::WATERSHED)
		//{
			// Prepare for region partitioning, by calculating distance field along the walkable surface.
			if (!rcBuildDistanceField(state.BuildContext.Ptr(), *state.CompactHeightfield->Ptr))
			{
				result.Error = "Navigation mesh bake failed: Could not build distance field.";
				return false;
			}

			// Partition the walkable surface into contiguous regions.
			if (!rcBuildRegions(state.BuildContext.Ptr(), *state.CompactHeightfield->Ptr, 0, state.Config.minRegionArea, state.Config.mergeRegionArea))
			{
				result.Error = "Navigation mesh bake failed: Could not build watershed regions.";
				return false;
			}
		//}
		//else if (partitionType == SamplePartitionType::MONOTONE)
		//{
		//	// Partition the walkable surface into contiguous regions.
		//	// Monotone partitioning does not need distancefield.
		//	if (!rcBuildRegionsMonotone(buildContext, *compactHeightfield, 0, config.minRegionArea, config.mergeRegionArea))
		//	{
		//		buildContext->log(RC_LOG_ERROR, "buildNavigation: Could not build monotone regions.");
		//		return false;
		//	}
		//}
		//else  // SamplePartitionType::LAYERS
		//{
		//	// Partition the walkable surface into contiguous regions.
		//	// Layer partitioning does not need distancefield.
		//	if (!rcBuildLayerRegions(buildContext, *compactHeightfield, 0, config.minRegionArea))
		//	{
		//		buildContext->log(RC_LOG_ERROR, "buildNavigation: Could not build layer regions.");
		//		return false;
		//	}
		//}

		return true;
	}

	bool NavigationMeshBuilder::TraceAndSimplifyContours(BuildPipelineState& state, BuildResult& result)
	{
		// 2.4: Trace and simplify region contours.
		// Create contour.
		state.ContourSet = ScopedPtr<ContourSetResource>::Create();
		if (!state.ContourSet || !state.ContourSet->Ptr)
		{
			result.Error = "Navigation mesh bake failed: Out of memory 'contourSet'.";
			return false;
		}
		if (!rcBuildContours(state.BuildContext.Ptr(), *state.CompactHeightfield->Ptr, state.Config.maxSimplificationError, state.Config.maxEdgeLen, *state.ContourSet->Ptr))
		{
			result.Error = "Navigation mesh bake failed: Could not create contours.";
			return false;
		}
		return true;
	}

	bool NavigationMeshBuilder::TriangulateContoursToPolyMesh(BuildPipelineState& state, BuildResult& result)
	{
		//
		// Step 6. Triangulate contours to build navmesh polygons.
		//
		state.PolyMesh = ScopedPtr<PolyMeshResource>::Create();
		if (!state.PolyMesh || !state.PolyMesh->Ptr)
		{
			result.Error = "Navigation mesh bake failed: Out of memory 'polyMesh'.";
			return false;
		}
		if (!rcBuildPolyMesh(state.BuildContext.Ptr(), *state.ContourSet->Ptr, state.Config.maxVertsPerPoly, *state.PolyMesh->Ptr))
		{
			result.Error = "Navigation mesh bake failed: Could not triangulate contours.";
			return false;
		}
		return true;
	}

	bool NavigationMeshBuilder::BuildPolyMeshDetail(BuildPipelineState& state, BuildResult& result)
	{
		// 2.6: Create a navmesh from the triangulated polygons.
		// Calculates additional information necessary to run pathing queries.
		state.DetailMesh = ScopedPtr<PolyMeshDetailResource>::Create();
		if (!state.DetailMesh || !state.DetailMesh->Ptr)
		{
			result.Error = "Navigation mesh bake failed: Out of memory 'detailMesh'.";
			return false;
		}
		if (!rcBuildPolyMeshDetail(
			state.BuildContext.Ptr(),
			*state.PolyMesh->Ptr,
			*state.CompactHeightfield->Ptr,
			state.Config.detailSampleDist,
			state.Config.detailSampleMaxError,
			*state.DetailMesh->Ptr))
		{
			result.Error = "Navigation mesh bake failed: Could not build detail mesh.";
			return false;
		}
		return true;
	}

	bool NavigationMeshBuilder::CreateDetourNavMesh(BuildPipelineState& state, BuildResult& result)
	{
		// At this point the navigation mesh data is ready to use.
		// See duDebugDrawPolyMesh or dtCreateNavMeshData as examples how to access
		// the navmesh data.

		//
		// (Optional) Step 8. Create Detour data from Recast poly mesh.
		//

		// The GUI may allow more max points per polygon than Detour can handle.
		// Only build the detour navmesh if we do not exceed the limit.
		if (state.Config.maxVertsPerPoly <= DT_VERTS_PER_POLYGON)
		{
			unsigned char* navData = 0;
			int navDataSize = 0;

			// Detour queries use polygon flags for filtering. Ensure walkable polys have a non-zero flag.
			for (int i = 0; i < state.PolyMesh->Ptr->npolys; ++i)
			{
				state.PolyMesh->Ptr->flags[i] = (state.PolyMesh->Ptr->areas[i] == RC_NULL_AREA) ? 0 : 0x1;
			}

			// Update poly flags from areas.
			//for (int i = 0; i < polyMesh->npolys; ++i)
			//{
			//	if (polyMesh->areas[i] == RC_WALKABLE_AREA)
			//	{
			//		polyMesh->areas[i] = SAMPLE_POLYAREA_GROUND;
			//	}

			//	if (polyMesh->areas[i] == SAMPLE_POLYAREA_GROUND || polyMesh->areas[i] == SAMPLE_POLYAREA_GRASS ||
			//		polyMesh->areas[i] == SAMPLE_POLYAREA_ROAD)
			//	{
			//		polyMesh->flags[i] = SAMPLE_POLYFLAGS_WALK;
			//	}
			//	else if (polyMesh->areas[i] == SAMPLE_POLYAREA_WATER)
			//	{
			//		polyMesh->flags[i] = SAMPLE_POLYFLAGS_SWIM;
			//	}
			//	else if (polyMesh->areas[i] == SAMPLE_POLYAREA_DOOR)
			//	{
			//		polyMesh->flags[i] = SAMPLE_POLYFLAGS_WALK | SAMPLE_POLYFLAGS_DOOR;
			//	}
			//}

			dtNavMeshCreateParams params;
			memset(&params, 0, sizeof(params));
			params.verts = state.PolyMesh->Ptr->verts;
			params.vertCount = state.PolyMesh->Ptr->nverts;
			params.polys = state.PolyMesh->Ptr->polys;
			params.polyAreas = state.PolyMesh->Ptr->areas;
			params.polyFlags = state.PolyMesh->Ptr->flags;
			params.polyCount = state.PolyMesh->Ptr->npolys;
			params.nvp = state.PolyMesh->Ptr->nvp;
			params.detailMeshes = state.DetailMesh->Ptr->meshes;
			params.detailVerts = state.DetailMesh->Ptr->verts;
			params.detailVertsCount = state.DetailMesh->Ptr->nverts;
			params.detailTris = state.DetailMesh->Ptr->tris;
			params.detailTriCount = state.DetailMesh->Ptr->ntris;
			// No authored off-mesh links in v1.
			params.offMeshConVerts = nullptr;
			params.offMeshConRad = nullptr;
			params.offMeshConDir = nullptr;
			params.offMeshConAreas = nullptr;
			params.offMeshConFlags = nullptr;
			params.offMeshConUserID = nullptr;
			params.offMeshConCount = 0;
			params.walkableHeight = state.Settings.AgentHeight;
			params.walkableRadius = state.Settings.AgentRadius;
			params.walkableClimb = state.Settings.AgentMaxClimb;
			rcVcopy(params.bmin, state.PolyMesh->Ptr->bmin);
			rcVcopy(params.bmax, state.PolyMesh->Ptr->bmax);
			params.cs = state.Config.cs;
			params.ch = state.Config.ch;
			params.buildBvTree = true;

			// 3) Fill dtNavMeshCreateParams and call dtCreateNavMeshData(...).
			if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
			{
				result.Error = "Navigation mesh bake failed: Could not build Detour navmesh.";
				return false;
			}

			dtNavMesh* navMesh = dtAllocNavMesh();
			if (!navMesh)
			{
				dtFree(navData);
				result.Error = "Navigation mesh bake failed: Could not create Detour navmesh.";
				return false;
			}

			// Copy blob before navMesh takes ownership and may free navData.
			result.RawDataBlob.resize(navDataSize);
			memcpy(result.RawDataBlob.data(), navData, static_cast<size_t>(navDataSize));

			dtStatus status = navMesh->init(navData, navDataSize, DT_TILE_FREE_DATA);
			if (dtStatusFailed(status))
			{
				dtFreeNavMesh(navMesh);
				result.Error = "Navigation mesh bake failed: Could not init Detour navmesh.";
				return false;
			}

			dtNavMeshQuery* navQuery = dtAllocNavMeshQuery();
			if (!navQuery)
			{
				dtFreeNavMesh(navMesh);
				result.Error = "Navigation mesh bake failed: Could not allocate Detour navmesh query.";
				return false;
			}
			status = navQuery->init(navMesh, 2048);
			if (dtStatusFailed(status))
			{
				dtFreeNavMeshQuery(navQuery);
				dtFreeNavMesh(navMesh);
				result.Error = "Navigation mesh bake failed: Could not init Detour navmesh query.";
				return false;
			}

			result.Success = true;
			result.RuntimeNavMesh = navMesh;
			result.RuntimeNavMeshQuery = navQuery;
			result.Error = "";
			return true;
		}

		result.Error = "Navigation mesh bake failed: maxVertsPerPoly exceeds Detour limit.";
		return false;
	}

}