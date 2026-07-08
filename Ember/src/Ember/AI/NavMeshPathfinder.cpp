#include "ebpch.h"
#include "NavMeshPathfinder.h"

#include <DetourCommon.h>

namespace Ember {

	std::vector<Vector3f> NavMeshPathfinder::BuildNavMeshPath(dtNavMesh* navMesh, dtNavMeshQuery* navQuery, const Vector3f& startPos, const Vector3f& endPos)
	{
		std::vector<Vector3f> waypoints;
		if (!navMesh || !navQuery)
			return waypoints;

		dtQueryFilter filter;
		filter.setIncludeFlags(0x1);
		filter.setExcludeFlags(0);

		const float polyPickExt[3] = { 2.0f, 4.0f, 2.0f };

		float startPt[3] = { startPos.x, startPos.y, startPos.z };
		float endPt[3] = { endPos.x, endPos.y, endPos.z };

		dtPolyRef startRef = 0;
		dtPolyRef endRef = 0;
		dtStatus startStatus = navQuery->findNearestPoly(startPt, polyPickExt, &filter, &startRef, nullptr);
		dtStatus endStatus = navQuery->findNearestPoly(endPt, polyPickExt, &filter, &endRef, nullptr);
		if (dtStatusFailed(startStatus) || dtStatusFailed(endStatus))
			return waypoints;

		if (!startRef || !endRef)
			return waypoints;

		static constexpr int MAX_POLYS = 256;
		dtPolyRef polys[MAX_POLYS] = {};
		int polyCount = 0;
		if (dtStatusFailed(navQuery->findPath(startRef, endRef, startPt, endPt, &filter, polys, &polyCount, MAX_POLYS)) || polyCount <= 0)
			return waypoints;

		static constexpr int MAX_STRAIGHT_PATH = 256;
		float straightPath[MAX_STRAIGHT_PATH * 3] = {};
		unsigned char straightPathFlags[MAX_STRAIGHT_PATH] = {};
		dtPolyRef straightPathRefs[MAX_STRAIGHT_PATH] = {};
		int straightPathCount = 0;

		if (dtStatusFailed(navQuery->findStraightPath(
			startPt,
			endPt,
			polys,
			polyCount,
			straightPath,
			straightPathFlags,
			straightPathRefs,
			&straightPathCount,
			MAX_STRAIGHT_PATH)))
		{
			return waypoints;
		}

		waypoints.reserve(static_cast<size_t>(straightPathCount));
		for (int i = 0; i < straightPathCount; ++i)
		{
			waypoints.emplace_back(straightPath[i * 3 + 0], straightPath[i * 3 + 1], straightPath[i * 3 + 2]);
		}

		return waypoints;
	}

}