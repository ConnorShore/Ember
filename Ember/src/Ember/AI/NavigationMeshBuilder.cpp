#include "ebpch.h"
#include "NavigationMeshBuilder.h"

#include "Ember/Core/Application.h"
#include "Ember/Render/Mesh.h"
#include "Ember/Scene/Scene.h"

#include <Recast.h>
#include <DetourNavMeshBuilder.h>

#include <algorithm>

namespace Ember {
	namespace {
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

		// TODO: Have ability to filter either rendered geometry or physics colliders only (using rendered geometry for now).
		EB_CORE_ASSERT(navMeshEntity.ContainsComponent<NavigationMeshComponent>(), "Entity must contain navigation mesh component");
		const auto navBounds = BuildNavBoundsAABB(navMeshEntity);

		// 1. Extract all static mesh geometry from the scene that overlaps the nav bounds.
		std::vector<float> vertices;
		std::vector<int> triangles;
		vertices.reserve(4096);
		triangles.reserve(4096);

		auto& assetManager = Application::Instance().GetAssetManager();

		auto& registry = scene->GetRegistry();
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
			if (!AABBOverlaps(meshWorldAABB, navBounds))
				continue;

			auto localVerts = mesh->GetVertexPositions();
			auto localTris = mesh->GetTriangles();

			if (localVerts.size() < 3 || localTris.size() < 3)
				continue;

			const int baseVertex = static_cast<int>(vertices.size() / 3);

			for (size_t i = 0; i + 2 < localVerts.size(); i += 3)
			{
				const Vector3f localPos(localVerts[i], localVerts[i + 1], localVerts[i + 2]);
				const Vector3f worldPos = world * localPos;

				vertices.push_back(worldPos.x);
				vertices.push_back(worldPos.y);
				vertices.push_back(worldPos.z);
			}

			for (uint32_t idx : localTris)
			{
				triangles.push_back(baseVertex + static_cast<int>(idx));
			}
		}

		const int numVerts = static_cast<int>(vertices.size() / 3);
		const int numTris = static_cast<int>(triangles.size() / 3);
		if (numVerts == 0 || numTris == 0)
		{
			result.Error = "Navigation mesh bake failed: no static mesh geometry overlaps the nav bounds.";
			return result;
		}

		const float* verts = vertices.data();
		const int* tris = triangles.data();

		// 2) Build rcConfig from settings and run Recast pipeline.
		ScopedPtr<rcContext> buildContext = ScopedPtr<rcContext>::Create(false);

		rcConfig config{};
		memset(&config, 0, sizeof(config));
		config.cs = settings.CellSize;
		config.ch = settings.CellHeight;
		config.walkableSlopeAngle = settings.AgentMaxSlope;
		config.walkableHeight = static_cast<int>(ceilf(settings.AgentHeight / config.ch));
		config.walkableClimb = static_cast<int>(floorf(settings.AgentMaxClimb / config.ch));
		config.walkableRadius = static_cast<int>(ceilf(settings.AgentRadius / config.cs));

		// TODO: Add config settings for rest of props once we get them from user, for now use default values
		config.maxEdgeLen = 12; // Default value
		config.maxSimplificationError = 1.3f; // Default value
		config.minRegionArea = 8; // Default value
		config.mergeRegionArea = 20; // Default value
		config.maxVertsPerPoly = 6; // Default value
		config.detailSampleDist = 6.0f; // Default value
		config.detailSampleMaxError = 1.0f; // Default value
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

		for (size_t i = 0; i + 2 < vertices.size(); i += 3)
		{
			boundsMin[0] = std::min(boundsMin[0], vertices[i + 0]);
			boundsMin[1] = std::min(boundsMin[1], vertices[i + 1]);
			boundsMin[2] = std::min(boundsMin[2], vertices[i + 2]);

			boundsMax[0] = std::max(boundsMax[0], vertices[i + 0]);
			boundsMax[1] = std::max(boundsMax[1], vertices[i + 1]);
			boundsMax[2] = std::max(boundsMax[2], vertices[i + 2]);
		}

		rcVcopy(config.bmin, boundsMin);
		rcVcopy(config.bmax, boundsMax);
		rcCalcGridSize(config.bmin, config.bmax, config.cs, &config.width, &config.height);

		// Rasterize input meshes.
		// Allocate voxel heightfield where we will store our rasterized input data.
		rcHeightfield* heightfield = nullptr;
		heightfield = rcAllocHeightfield();
		if (!heightfield)
		{
			result.Error = "Navigation mesh bake failed: Out of memory 'heightfield'.";
			return result;
		}

		if (!rcCreateHeightfield(
			buildContext.Ptr(),
			*heightfield,
			config.width,
			config.height,
			config.bmin,
			config.bmax,
			config.cs,
			config.ch))
		{
			result.Error = "Navigation mesh bake failed: Could not create solid heightfield.";
			return result;
		}

		// Allocate array that can hold triangle area types.
		// This is used to store terrain type information and to mark
		// triangles as unwalkable.
		// If you have multiple meshes you need to process, allocate
		// an array which can hold the max number of triangles you need to process.
		unsigned char* triAreas = new unsigned char[numTris];
		if (!triAreas)
		{
			result.Error = "Navigation mesh bake failed: Out of memory 'triAreas'.";
			return result;
		}
		memset(triAreas, 0, numTris * sizeof(unsigned char));

		// Record which triangles in the input mesh are walkable.
		// This information is recorded in triAreas
		rcMarkWalkableTriangles(buildContext.Ptr(), config.walkableSlopeAngle, verts, numVerts, tris, numTris, triAreas);

		// 2.1: Rasterize the input mesh
		// If your have multiple meshes, you can transform them, calculate the
		// terrain type for each mesh and rasterize them here.
		if (!rcRasterizeTriangles(buildContext.Ptr(), verts, numVerts, tris, triAreas, numTris, *heightfield, config.walkableClimb))
		{
			result.Error = "Navigation mesh bake failed: Could not rasterize triangles.";
			return result;
		}

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
			rcFilterLowHangingWalkableObstacles(buildContext.Ptr(), config.walkableClimb, *heightfield);
		}
		if (filterLedgeSpans)
		{
			rcFilterLedgeSpans(buildContext.Ptr(), config.walkableHeight, config.walkableClimb, *heightfield);
		}
		if (filterWalkableLowHeightSpans)
		{
			rcFilterWalkableLowHeightSpans(buildContext.Ptr(), config.walkableHeight, *heightfield);
		}

		// 2.3: Partition walkable surface into simple regions.
		// Compact the heightfield so that it is faster to work with.
		// This will result more cache coherent data.  This step will also
		// generate neighbor connection information between walkable cells.
		rcCompactHeightfield* compactHeightfield = rcAllocCompactHeightfield();
		if (!compactHeightfield)
		{
			result.Error = "Navigation mesh bake failed: Out of memory 'compactHeightfield'.";
			return result;
		}
		if (!rcBuildCompactHeightfield(
			buildContext.Ptr(),
			config.walkableHeight,
			config.walkableClimb,
			*heightfield,
			*compactHeightfield))
		{
			result.Error = "Navigation mesh bake failed: Could not build compact data.";
			return result;
		}

		// Erode the walkable area by agent radius.
		// This allows us to path an agent through the navmesh as if it was a single point
		if (!rcErodeWalkableArea(buildContext.Ptr(), config.walkableRadius, *compactHeightfield))
		{
			result.Error = "Navigation mesh bake failed: Could not erode walkable area.";
			return result;
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
			if (!rcBuildDistanceField(buildContext.Ptr(), *compactHeightfield))
			{
				result.Error = "Navigation mesh bake failed: Could not build distance field.";
				return result;
			}

			// Partition the walkable surface into contiguous regions.
			if (!rcBuildRegions(buildContext.Ptr(), *compactHeightfield, 0, config.minRegionArea, config.mergeRegionArea))
			{
				result.Error = "Navigation mesh bake failed: Could not build watershed regions.";
				return result;
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

		// 2.4: Trace and simplify region contours.
		// Create contour.
		rcContourSet* contourSet = rcAllocContourSet();
		if (!contourSet)
		{
			result.Error = "Navigation mesh bake failed: Out of memory 'contourSet'.";
			return result;
		}
		if (!rcBuildContours(buildContext.Ptr(), *compactHeightfield, config.maxSimplificationError, config.maxEdgeLen, *contourSet))
		{
			result.Error = "Navigation mesh bake failed: Could not create contours.";
			return result;
		}

		//
		// Step 6. Triangulate contours to build navmesh polygons.
		//
		rcPolyMesh* polyMesh = rcAllocPolyMesh();
		if (!polyMesh)
		{
			result.Error = "Navigation mesh bake failed: Out of memory 'polyMesh'.";
			return result;
		}
		if (!rcBuildPolyMesh(buildContext.Ptr(), *contourSet, config.maxVertsPerPoly, *polyMesh))
		{
			result.Error = "Navigation mesh bake failed: Could not triangulate contours.";
			return result;
		}

		// 2.6: Create a navmesh from the triangulated polygons.
		// Calculates additional information necessary to run pathing queries.
		rcPolyMeshDetail* detailMesh = rcAllocPolyMeshDetail();
		if (!detailMesh)
		{
			result.Error = "Navigation mesh bake failed: Out of memory 'detailMesh'.";
			return result;
		}
		if (!rcBuildPolyMeshDetail(
			buildContext.Ptr(),
			*polyMesh,
			*compactHeightfield,
			config.detailSampleDist,
			config.detailSampleMaxError,
			*detailMesh))
		{
			result.Error = "Navigation mesh bake failed: Could not build detail mesh.";
			return result;
		}

		// At this point the navigation mesh data is ready to use.
	// See duDebugDrawPolyMesh or dtCreateNavMeshData as examples how to access
	// the navmesh data.

	//
	// (Optional) Step 8. Create Detour data from Recast poly mesh.
	//

	// The GUI may allow more max points per polygon than Detour can handle.
	// Only build the detour navmesh if we do not exceed the limit.
		if (config.maxVertsPerPoly <= DT_VERTS_PER_POLYGON)
		{
			unsigned char* navData = 0;
			int navDataSize = 0;

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
			params.verts = polyMesh->verts;
			params.vertCount = polyMesh->nverts;
			params.polys = polyMesh->polys;
			params.polyAreas = polyMesh->areas;
			params.polyFlags = polyMesh->flags;
			params.polyCount = polyMesh->npolys;
			params.nvp = polyMesh->nvp;
			params.detailMeshes = detailMesh->meshes;
			params.detailVerts = detailMesh->verts;
			params.detailVertsCount = detailMesh->nverts;
			params.detailTris = detailMesh->tris;
			params.detailTriCount = detailMesh->ntris;
			//params.offMeshConVerts = inputGeometry->offmeshConnVerts.data();
			//params.offMeshConRad = inputGeometry->offmeshConnRadius.data();
			//params.offMeshConDir = inputGeometry->offmeshConnBidirectional.data();
			//params.offMeshConAreas = inputGeometry->offmeshConnArea.data();
			//params.offMeshConFlags = inputGeometry->offmeshConnFlags.data();
			//params.offMeshConUserID = inputGeometry->offmeshConnId.data();
			//params.offMeshConCount = static_cast<int>(inputGeometry->offmeshConnArea.size());
			params.walkableHeight = settings.AgentHeight;
			params.walkableRadius = settings.AgentRadius;
			params.walkableClimb = settings.AgentMaxClimb;
			rcVcopy(params.bmin, polyMesh->bmin);
			rcVcopy(params.bmax, polyMesh->bmax);
			params.cs = config.cs;
			params.ch = config.ch;
			params.buildBvTree = true;

			// 3) Fill dtNavMeshCreateParams and call dtCreateNavMeshData(...).
			if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
			{
				result.Error = "Navigation mesh bake failed: Could not build Detour navmesh.";
				return result;
			}

			dtNavMesh* navMesh = dtAllocNavMesh();
			if (!navMesh)
			{
				dtFree(navData);
				result.Error = "Navigation mesh bake failed: Could not create Detour navmesh.";
				return result;
			}

			// Copy blob before navMesh takes ownership and may free navData.
			result.RawDataBlob.resize(navDataSize);
			memcpy(result.RawDataBlob.data(), navData, static_cast<size_t>(navDataSize));

			dtStatus status = navMesh->init(navData, navDataSize, DT_TILE_FREE_DATA);
			if (dtStatusFailed(status))
			{
				dtFreeNavMesh(navMesh);
				result.Error = "Navigation mesh bake failed: Could not init Detour navmesh.";
				return result;
			}

			result.Success = true;
			result.RuntimeNavMesh = navMesh;
			result.Error = "";
		}

		// TODO: Clean up all pointers
		delete[] triAreas;
		triAreas = nullptr;
		rcFreeHeightField(heightfield);
		heightfield = nullptr;
		rcFreeCompactHeightfield(compactHeightfield);
		compactHeightfield = nullptr;
		rcFreeContourSet(contourSet);
		contourSet = nullptr;
		rcFreePolyMesh(polyMesh);
		polyMesh = nullptr;
		rcFreePolyMeshDetail(detailMesh);
		detailMesh = nullptr;

		return result;
	}

}