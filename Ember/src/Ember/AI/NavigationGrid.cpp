#include "ebpch.h"
#include "NavigationGrid.h"

#include "Ember/Scene/Scene.h"
#include "Ember/ECS/System/PhysicsSystem.h"
#include "Ember/Render/DebugRenderer.h"

namespace Ember {

	constexpr Vector3f ConnectionLineColor = Vector3f(0.6f);
	constexpr Vector3f WalkableNodeColor = Vector3f(0.0f, 0.5f, 1.0f);
	constexpr Vector3f UnwalkableNodeColor = Vector3f(1.0f, 0.0f, 0.0f);

	std::vector<std::vector<NavNode>> NavigationGrid::Generate(const Vector3f& center, float gridSizeX, float gridSizeY, float nodeSpacing)
	{
		std::vector<std::vector<NavNode>> grid;

		// Calculate grid origin and number of nodes based on size and spacing
		Vector3f gridOrigin = center - Vector3f(gridSizeX / 2.0f, 0.0f, gridSizeY / 2.0f);	// Bottom left corner of the grid in world space
		Vector3f gridEnd = center + Vector3f(gridSizeX / 2.0f, 0.0f, gridSizeY / 2.0f);		// Top right corner of the grid in world space
		int numNodesX = static_cast<int>(gridSizeX / nodeSpacing);
		int numNodesY = static_cast<int>(gridSizeY / nodeSpacing);

		// Generate grid of nodes based on size and spacing
		for (int x = 0; x < numNodesX; ++x)
		{
			std::vector<NavNode> column;
			for (int y = 0; y < numNodesY; ++y)
			{
				Vector3f nodePos = gridOrigin + Vector3f(x * nodeSpacing, 0.0f, y * nodeSpacing);
				Vector3f centeredNodePos = nodePos + Vector3f(nodeSpacing / 2.0f, 0.0f, nodeSpacing / 2.0f); // Center the node in its grid cell
				NavNode node = { centeredNodePos, x, y, true };
				column.push_back(node);
			}
			grid.push_back(column);
		}

		// Check each node to verify it doesn't intersect with any colliders in the world and mark it as walkable or not
		auto physicsSystem = Application::Instance().GetSystem<PhysicsSystem>();
		for (auto& column : grid)
		{
			for (auto& node : column)
			{
				//auto overlapData = physicsSystem->TestOverlapSphere(node.WorldPosition, nodeSpacing * 0.45f, {});
				// TODO: Pass in rotation (float: y-rot)
				float width = nodeSpacing * 0.45f;
				auto overlapData = physicsSystem->TestOverlapBox(node.WorldPosition, Vector3f(0.0f), Vector3f(width, 0.1f, width), {}, CollisionFilterPreset::Environment);
				node.IsWalkable = !overlapData.HasHit;
			}
		}

		// Return final grid of nodes for use in A* pathfinding
		return grid;
	}

	void NavigationGrid::RenderGeneratedGrid(const std::vector<std::vector<NavNode>>& grid, bool selected /* = false */)
	{
		// Draw grid with walkable nodes in green and unwalkable nodes in red
		// Connect nodes with lines to visualize the grid structure
		// If a node is unwalkable, don't draw lines to its neighbors to visualize that it's not traversable
		float alpha = selected ? 1.0f : 0.5f;
		for (const auto& column : grid)
		{
			for (const auto& node : column)
			{
				DebugRenderer::DrawOctahedron(node.WorldPosition, 0.25f, node.IsWalkable ? Vector4f(WalkableNodeColor, alpha) : Vector4f(UnwalkableNodeColor, alpha));

				if (node.IsWalkable)
				{
					// Connect to right neighbor
					if (node.GridX < grid.size() - 1 && grid[node.GridX + 1][node.GridY].IsWalkable)
						DebugRenderer::DrawLine(node.WorldPosition, grid[node.GridX + 1][node.GridY].WorldPosition, Vector4f(ConnectionLineColor, alpha));
					// Connect to top neighbor
					if (node.GridY < column.size() - 1 && grid[node.GridX][node.GridY + 1].IsWalkable)
						DebugRenderer::DrawLine(node.WorldPosition, grid[node.GridX][node.GridY + 1].WorldPosition, Vector4f(ConnectionLineColor, alpha));
				}
			}
		}
	}

	void NavigationGrid::RenderUngeneratedGrid(const Vector3f& center, float gridSizeX, float gridSizeY, bool selected /* = false */)
	{
		// Draw a wireframe box to visualize the bounds of the grid before it's generated
		// Its x, z should correspond to gridsizeX and gridsizeY
		float alpha = selected ? 1.0f : 0.5f;
		DebugRenderer::DrawCube(center, Vector3f(gridSizeX, 0.5f, gridSizeY), Vector4f(0.0f, 0.5f, 1.0f, alpha));
	}

}