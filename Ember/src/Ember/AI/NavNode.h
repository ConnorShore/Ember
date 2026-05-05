#pragma once

#include "Ember/Math/Math.h"

namespace Ember {

	// Node for A* navigation
	struct NavNode
	{
		Vector3f WorldPosition;
		int GridX, GridY;
		bool IsWalkable;

		// A* Math Variables
		float GCost = 0; // Distance from start
		float HCost = 0; // Estimated distance to target
		float FCost() const { return GCost + HCost; }
		NavNode* Parent = nullptr; // Used to trace the path backwards once found

		NavNode() = default;
		NavNode(const Vector3f& worldPos, int gridX, int gridY, bool isWalkable)
			: WorldPosition(worldPos), GridX(gridX), GridY(gridY), IsWalkable(isWalkable) {}
		NavNode(const NavNode&) = default;
	};

}