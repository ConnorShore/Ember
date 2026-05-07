#include "ebpch.h"
#include "AStar.h"

#include <vector>
#include <queue>

namespace Ember {

	std::vector<Vector3f> AStar::AStarPath(const Vector3f& start, const Vector3f& goal, std::vector<std::vector<NavNode>>& grid)
	{
		uint32_t gridWidth = grid.size();
		uint32_t gridHeight = grid[0].size();

		// 3. The Scratch Map: A parallel 2D vector initialized to default values.
		// This gives us instant O(1) lookups for G-costs and Parents without needing a map hash!
		std::vector<std::vector<NavNodeScratch>> scratchGrid(gridWidth, std::vector<NavNodeScratch>(gridHeight));

		// Wire up the pointers
		for (int x = 0; x < gridWidth; ++x) {
			for (int y = 0; y < gridHeight; ++y) {
				scratchGrid[x][y].Node = &grid[x][y];
			}
		}

		// The Open Set: A Priority Queue of pointers to our scratch grid
		std::priority_queue<NavNodeScratch*, std::vector<NavNodeScratch*>, CompareNode> openSet;

		const NavNode* startNode = FindClosestNode(start, grid);
		const NavNode* goalNode = FindClosestNode(goal, grid);

		// Setup Start Node
		NavNodeScratch* startScratch = &scratchGrid[startNode->GridX][startNode->GridY];
		startScratch->GCost = 0.0f;
		startScratch->HCost = Heuristic(*startNode, *goalNode);

		openSet.push(startScratch);

		while (!openSet.empty())
		{
			// Get the node with the lowest F-cost
			NavNodeScratch* current = openSet.top();
			openSet.pop();

			if (current->IsClosed)
				continue;

			// Build path if we are at the goal
			if (current->Node->GridX == goalNode->GridX && current->Node->GridY == goalNode->GridY)
				return ReconstructPath(current, scratchGrid);
			
			current->IsClosed = true;

			// Loop through neighbors
			for (int dx = -1; dx <= 1; ++dx) {
				for (int dy = -1; dy <= 1; ++dy) {
					if (dx == 0 && dy == 0)
						continue; // Skip self

					int neighborX = current->Node->GridX + dx;
					int neighborY = current->Node->GridY + dy;

					// Check bounds
					if (neighborX < 0 || neighborX >= gridWidth || neighborY < 0 || neighborY >= gridHeight)
						continue;

					NavNodeScratch& neighborScratch = scratchGrid[neighborX][neighborY];
					NavNode neighborNode = *neighborScratch.Node;
					if (!neighborNode.IsWalkable || neighborScratch.IsClosed)
						continue;

					// Prevent clipping through diagonal corners
					if (dx != 0 && dy != 0) // If we are moving diagonally...
					{
						bool walkX = grid[current->Node->GridX + dx][current->Node->GridY].IsWalkable;
						bool walkY = grid[current->Node->GridX][current->Node->GridY + dy].IsWalkable;

						// If EITHER adjacent straight nodes are walls, the diagonal is blocked!
						if (!walkX || !walkY)
							continue;
					}

					float tentativeGCost = current->GCost + Heuristic(*current->Node, neighborNode);
					if (tentativeGCost < neighborScratch.GCost) {
						neighborScratch.Parent = current->Node;
						neighborScratch.GCost = tentativeGCost;
						neighborScratch.HCost = Heuristic(neighborNode, *goalNode);
						openSet.push(&neighborScratch);
					}
				}
			}
		}

		return {}; // No path found
	}

	std::vector<Vector3f> AStar::ReconstructPath(NavNodeScratch* endNode, std::vector<std::vector<NavNodeScratch>>& grid)
	{
		std::vector<Vector3f> path;
		NavNodeScratch* current = endNode;
		while (current != nullptr)
		{
			path.push_back(current->Node->WorldPosition);
			if (current->Parent == nullptr)
				break;
			int parentX = current->Parent->GridX;
			int parentY = current->Parent->GridY;
			current = &grid[parentX][parentY];
		}

		return std::vector<Vector3f>(path.rbegin(), path.rend()); // Reverse the path to get it from start to goal
	}

	float AStar::Heuristic(const NavNode& a, const NavNode& b)
	{
		// Use octile distance as the heuristic, since we allow diagonal movement in our grid
		// This is an admissible heuristic that accurately reflects the actual movement cost on the grid, ensuring optimal paths
		int dx = std::abs(a.GridX - b.GridX);
		int dy = std::abs(a.GridY - b.GridY);

		if (dx > dy)
			return 14.0f * dy + 10.0f * (dx - dy);
		return 14.0f * dx + 10.0f * (dy - dx);
	}

	NavNode* AStar::FindClosestNode(const Vector3f& position, std::vector<std::vector<NavNode>>& grid)
	{
		// Find closest walkable node in the grid to the given position
		uint32_t gridWidth = grid.size();
		uint32_t gridHeight = grid[0].size();
		float gridMinX = grid[0][0].WorldPosition.x;
		float gridMinY = grid[0][0].WorldPosition.z;
		float gridMaxX = grid[gridWidth - 1][0].WorldPosition.x;
		float gridMaxY = grid[0][gridHeight - 1].WorldPosition.z;

		uint32_t closestX = static_cast<uint32_t>(std::clamp((position.x - gridMinX) / (gridMaxX - gridMinX) * gridWidth, 0.0f, static_cast<float>(gridWidth - 1)));
		uint32_t closestY = static_cast<uint32_t>(std::clamp((position.z - gridMinY) / (gridMaxY - gridMinY) * gridHeight, 0.0f, static_cast<float>(gridHeight - 1)));

		return &grid[closestX][closestY];
	}

}