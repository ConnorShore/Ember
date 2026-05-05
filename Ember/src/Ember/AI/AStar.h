#pragma once

#include "NavNode.h"
#include "Ember/Math/Math.h"

#include <vector>

namespace Ember {

	class AStar
	{
	public:
		static std::vector<Vector3f> AStarPath(const Vector3f& start, const Vector3f& goal, std::vector<std::vector<NavNode>>& grid);

	private:
		struct NavNodeScratch
		{
			const NavNode* Node = nullptr;
			const NavNode* Parent = nullptr;

			float GCost = FLT_MAX; // Default to infinity
			float HCost = 0;
			float FCost() const { return GCost + HCost; }

			bool IsClosed = false;
		};

		// The Comparator for the Priority Queue
		// This tells the queue to keep the node with the LOWEST F-cost at the top.
		struct CompareNode
		{
			bool operator()(const NavNodeScratch* a, const NavNodeScratch* b) const
			{
				if (a->FCost() == b->FCost())
					return a->HCost > b->HCost; // Tie-breaker: pick the one closest to the goal
				return a->FCost() > b->FCost();
			}
		};

	private:
		static std::vector<Vector3f> ReconstructPath(NavNodeScratch* endNode, std::vector<std::vector<NavNodeScratch>>& grid);
		static float Heuristic(const NavNode& a, const NavNode& b);

		static NavNode* FindClosestNode(const Vector3f& position, std::vector<std::vector<NavNode>>& grid);
	};


}