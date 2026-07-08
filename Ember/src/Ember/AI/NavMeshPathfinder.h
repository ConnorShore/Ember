#pragma once

#include "Ember/Math/Math.h"

#include <DetourNavMesh.h>
#include <DetourNavMeshQuery.h>

#include <vector>

namespace Ember {

	class NavMeshPathfinder
	{
	public:
		static std::vector<Vector3f> BuildNavMeshPath(dtNavMesh* navMesh, dtNavMeshQuery* navQuery,	const Vector3f& startPos, const Vector3f& endPos);
	};

}