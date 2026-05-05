#pragma once

#include "Ember/Math/Math.h"

namespace Ember {

	// Node for A* navigation
	struct NavNode
	{
		Vector3f WorldPosition;
		int GridX, GridY;
		bool IsWalkable;

		NavNode() = default;
		NavNode(const Vector3f& worldPos, int gridX, int gridY, bool isWalkable)
			: WorldPosition(worldPos), GridX(gridX), GridY(gridY), IsWalkable(isWalkable) {}
		NavNode(const NavNode&) = default;
	};

}