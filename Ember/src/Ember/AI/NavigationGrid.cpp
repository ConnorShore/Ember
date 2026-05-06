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

		// Calculate node counts safely using std::round to prevent float truncation!
		int numNodesX = static_cast<int>(std::round(gridSizeX / nodeSpacing));
		int numNodesY = static_cast<int>(std::round(gridSizeY / nodeSpacing));

		// Calculate the ACTUAL physical dimensions the generated nodes will occupy
		float actualSizeX = numNodesX * nodeSpacing;
		float actualSizeY = numNodesY * nodeSpacing;

		// Center the origin based on the actual size, not the requested gridSize
		Vector3f gridOrigin = center - Vector3f(actualSizeX / 2.0f, 0.0f, actualSizeY / 2.0f);

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

		// Check each node to verify it doesn't intersect with any colliders
		auto physicsSystem = Application::Instance().GetSystem<PhysicsSystem>();
		for (auto& column : grid)
		{
			for (auto& node : column)
			{
				float width = nodeSpacing * 0.45f;
				auto overlapData = physicsSystem->TestOverlapBox(node.WorldPosition, Vector3f(0.0f), Vector3f(width, 0.1f, width), {}, CollisionFilterPreset::Environment);
				node.IsWalkable = overlapData;	// If overlap data exists
			}
		}

		return grid;
	}

	void NavigationGrid::RenderGeneratedGrid(const std::vector<std::vector<NavNode>>& grid, bool selected /* = false */)
	{
		if (grid.empty() || grid[0].empty())
			return;

		// Draw grid with walkable nodes in green and unwalkable nodes in red
		// Connect nodes with lines to visualize the grid structure
		// If a node is unwalkable, don't draw lines to its neighbors to visualize that it's not traversable
		float alpha = selected ? 1.0f : 0.5f;

		// Infer spacing from generated node positions so debug visuals always match the generated grid.
		float inferredSpacing = 1.0f;
		if (grid.size() > 1 && !grid[1].empty())
			inferredSpacing = glm::length(grid[1][0].WorldPosition - grid[0][0].WorldPosition);
		else if (grid[0].size() > 1)
			inferredSpacing = glm::length(grid[0][1].WorldPosition - grid[0][0].WorldPosition);

		if (inferredSpacing <= 0.0001f)
			inferredSpacing = 1.0f;

		float octahedronSize = inferredSpacing * 0.25f;
		for (size_t x = 0; x < grid.size(); x++)
		{
			const auto& column = grid[x];
			for (size_t y = 0; y < column.size(); y++)
			{
				const auto& node = column[y];

				DebugRenderer::DrawOctahedron(node.WorldPosition, octahedronSize, node.IsWalkable ? Vector4f(WalkableNodeColor, alpha) : Vector4f(UnwalkableNodeColor, alpha));

				if (node.IsWalkable)
				{
					// Connect to right neighbor
					if (x + 1 < grid.size() && y < grid[x + 1].size() && grid[x + 1][y].IsWalkable)
						DebugRenderer::DrawLine(node.WorldPosition, grid[x + 1][y].WorldPosition, Vector4f(ConnectionLineColor, alpha));
					// Connect to top neighbor
					if (y + 1 < column.size() && grid[x][y + 1].IsWalkable)
						DebugRenderer::DrawLine(node.WorldPosition, grid[x][y + 1].WorldPosition, Vector4f(ConnectionLineColor, alpha));
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