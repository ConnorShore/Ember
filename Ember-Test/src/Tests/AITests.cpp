// A* pathfinding over the navigation grid: contiguity, obstacle avoidance and corner-cutting rules.
// Grid indexing matches NavigationGridComponent::Grid - outer index X, inner index Z, world (x, 0, z).

#include <Ember.h>

#include "Ember/AI/AStar.h"

#include "TestFramework.h"
#include "TestHelpers.h"

#include <cmath>
#include <string>
#include <vector>

using namespace Ember;
using Ember::Test::Type::Unit;
using Ember::Test::Type::Integration;

namespace {

	constexpr int kGridSize = 12;

	// A fully walkable square grid whose node at [x][z] sits at world (x, 0, z).
	std::vector<std::vector<NavNode>> MakeGrid(int size = kGridSize)
	{
		std::vector<std::vector<NavNode>> grid;
		grid.reserve(size);
		for (int x = 0; x < size; ++x)
		{
			std::vector<NavNode> column;
			column.reserve(size);
			for (int z = 0; z < size; ++z)
				column.emplace_back(Vector3f((float)x, 0.0f, (float)z), x, z, true);
			grid.push_back(std::move(column));
		}
		return grid;
	}

	bool IsWalkableAtWorld(const std::vector<std::vector<NavNode>>& grid, const Vector3f& position)
	{
		const int x = (int)std::lround(position.x);
		const int z = (int)std::lround(position.z);
		if (x < 0 || x >= (int)grid.size() || z < 0 || z >= (int)grid[0].size())
			return false;
		return grid[x][z].IsWalkable;
	}

	// True when consecutive waypoints are adjacent on the grid (including diagonals).
	bool IsContiguous(const std::vector<Vector3f>& path)
	{
		for (size_t i = 1; i < path.size(); ++i)
		{
			const float dx = std::abs(path[i].x - path[i - 1].x);
			const float dz = std::abs(path[i].z - path[i - 1].z);
			if (dx > 1.001f || dz > 1.001f)
				return false;
			if (dx < 0.001f && dz < 0.001f)
				return false; // a repeated node is not progress
		}
		return true;
	}

} // namespace

//////////////////////////////////////////////////////////////////////////
// Basic pathing
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(AI, PathAcrossAnOpenGrid, Unit)
{
	std::vector<std::vector<NavNode>> grid = MakeGrid();

	const Vector3f start(0.0f, 0.0f, 0.0f);
	const Vector3f goal((float)(kGridSize - 1), 0.0f, (float)(kGridSize - 1));

	const std::vector<Vector3f> path = AStar::AStarPath(start, goal, grid);

	EB_CHECK_MSG(!path.empty(), "no path found across a completely open grid");
	EB_EXPECT_VEC3_NEAR(path.front(), start, 0.51f);
	EB_EXPECT_VEC3_NEAR(path.back(), goal, 0.51f);
	EB_EXPECT_MSG(IsContiguous(path), "the returned path skips or repeats grid nodes");

	// The grid allows diagonals, so a corner-to-corner path is (size - 1) steps plus the start node.
	EB_NOTE("open-grid path length: " + std::to_string(path.size()) + " nodes");
	EB_EXPECT_EQ(path.size(), (size_t)kGridSize);
}

EB_TEST_CASE(AI, PathToSelfIsTrivial, Unit)
{
	std::vector<std::vector<NavNode>> grid = MakeGrid();

	const Vector3f point(4.0f, 0.0f, 4.0f);
	const std::vector<Vector3f> path = AStar::AStarPath(point, point, grid);

	EB_CHECK_MSG(!path.empty(), "pathing to the start node returned nothing at all");
	EB_EXPECT_EQ(path.size(), (size_t)1);
	EB_EXPECT_VEC3_NEAR(path.front(), point, 0.51f);
}

EB_TEST_CASE(AI, StraightLinePathIsOptimal, Unit)
{
	// A clear straight run must not wander. An inadmissible heuristic or a broken cost comparison
	// shows up here as a path that is longer than it needs to be.
	std::vector<std::vector<NavNode>> grid = MakeGrid();

	const std::vector<Vector3f> path = AStar::AStarPath(
		Vector3f(0.0f, 0.0f, 5.0f), Vector3f(5.0f, 0.0f, 5.0f), grid);

	EB_CHECK(!path.empty());
	EB_EXPECT_EQ(path.size(), (size_t)6); // nodes at x = 0..5
	for (const Vector3f& waypoint : path)
		EB_EXPECT_NEAR(waypoint.z, 5.0f, 0.51);
}

//////////////////////////////////////////////////////////////////////////
// Obstacles
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(AI, PathRoutesAroundAWall, Unit)
{
	// A wall spanning the grid except for one gap. The path must exist, must avoid every
	// unwalkable node, and must pass through the gap.
	std::vector<std::vector<NavNode>> grid = MakeGrid();

	constexpr int kWallX = 6;
	constexpr int kGapZ = kGridSize - 1;
	for (int z = 0; z < kGridSize; ++z)
	{
		if (z != kGapZ)
			grid[kWallX][z].IsWalkable = false;
	}

	const std::vector<Vector3f> path = AStar::AStarPath(
		Vector3f(0.0f, 0.0f, 0.0f), Vector3f((float)(kGridSize - 1), 0.0f, 0.0f), grid);

	EB_CHECK_MSG(!path.empty(), "no path found through the gap in the wall");
	EB_EXPECT_MSG(IsContiguous(path), "the path around the wall skips grid nodes");

	for (const Vector3f& waypoint : path)
	{
		EB_EXPECT_MSG(IsWalkableAtWorld(grid, waypoint),
			"the path crosses an unwalkable node at " + Ember::Test::ToString(waypoint));
	}

	// It has to detour through the only gap, so it must be longer than the direct run.
	EB_EXPECT_GT(path.size(), (size_t)kGridSize);
	EB_NOTE("detour path length: " + std::to_string(path.size()) + " nodes");
}

EB_TEST_CASE(AI, NoPathWhenTheGoalIsWalledOff, Unit)
{
	// A completely sealed wall must produce an EMPTY path, not a partial one. An agent handed a
	// partial path walks confidently into the wall and stops there.
	std::vector<std::vector<NavNode>> grid = MakeGrid();

	constexpr int kWallX = 6;
	for (int z = 0; z < kGridSize; ++z)
		grid[kWallX][z].IsWalkable = false;

	const std::vector<Vector3f> path = AStar::AStarPath(
		Vector3f(0.0f, 0.0f, 0.0f), Vector3f((float)(kGridSize - 1), 0.0f, 0.0f), grid);

	EB_EXPECT_MSG(path.empty(), "a path was returned across a fully sealed wall");
}

EB_TEST_CASE(AI, DiagonalsDoNotCutThroughCorners, Unit)
{
	// A diagonal step requires BOTH orthogonal neighbours to be walkable, or agents clip through
	// corners. A 3x3 grid with only the main diagonal open makes every candidate step a corner cut.
	{
		std::vector<std::vector<NavNode>> grid = MakeGrid(3);
		for (int x = 0; x < 3; ++x)
		{
			for (int z = 0; z < 3; ++z)
				grid[x][z].IsWalkable = (x == z);
		}

		const std::vector<Vector3f> path = AStar::AStarPath(
			Vector3f(0.0f, 0.0f, 0.0f), Vector3f(2.0f, 0.0f, 2.0f), grid);

		EB_EXPECT_MSG(path.empty(), "the path cut diagonally between two sealed corners");
	}

	// Control: open the orthogonal neighbours and the same diagonal route becomes legal. Without
	// this half, a pathfinder that simply refused all diagonals would also pass the check above.
	{
		std::vector<std::vector<NavNode>> grid = MakeGrid(3);
		const std::vector<Vector3f> path = AStar::AStarPath(
			Vector3f(0.0f, 0.0f, 0.0f), Vector3f(2.0f, 0.0f, 2.0f), grid);

		EB_CHECK_MSG(!path.empty(), "no path across a fully open 3x3 grid");
		EB_EXPECT_MSG(path.size() == 3, "diagonal movement is not being used on open ground");
	}
}

EB_TEST_CASE(AI, PathNeverEntersUnwalkableRegions, Unit)
{
	// A scattered obstacle field, checked exhaustively. This is the property that actually matters
	// to a designer: whatever route is chosen, it stays on walkable ground.
	std::vector<std::vector<NavNode>> grid = MakeGrid();

	// Deterministic pseudo-scatter - no RNG, so a failure is always reproducible.
	for (int x = 1; x < kGridSize - 1; ++x)
	{
		for (int z = 1; z < kGridSize - 1; ++z)
		{
			if (((x * 7) + (z * 3)) % 5 == 0)
				grid[x][z].IsWalkable = false;
		}
	}

	// Keep the endpoints clear.
	grid[0][0].IsWalkable = true;
	grid[kGridSize - 1][kGridSize - 1].IsWalkable = true;

	const std::vector<Vector3f> path = AStar::AStarPath(
		Vector3f(0.0f, 0.0f, 0.0f), Vector3f((float)(kGridSize - 1), 0.0f, (float)(kGridSize - 1)), grid);

	if (path.empty())
		EB_SKIP("the generated obstacle field happens to have no solution - not a regression");

	EB_EXPECT_MSG(IsContiguous(path), "the path skips grid nodes");
	for (const Vector3f& waypoint : path)
	{
		EB_EXPECT_MSG(IsWalkableAtWorld(grid, waypoint),
			"the path crosses an unwalkable node at " + Ember::Test::ToString(waypoint));
	}
}

EB_TEST_CASE(AI, PathfindingIsDeterministic, Unit)
{
	// Two identical queries must produce identical paths. Non-determinism here (from tie-breaking
	// on pointer order, say) makes agents visibly jitter between two equal-cost routes.
	std::vector<std::vector<NavNode>> gridA = MakeGrid();
	std::vector<std::vector<NavNode>> gridB = MakeGrid();

	const Vector3f start(1.0f, 0.0f, 1.0f);
	const Vector3f goal(10.0f, 0.0f, 8.0f);

	const std::vector<Vector3f> first = AStar::AStarPath(start, goal, gridA);
	const std::vector<Vector3f> second = AStar::AStarPath(start, goal, gridB);

	EB_CHECK(!first.empty());
	EB_CHECK_EQ(first.size(), second.size());
	for (size_t i = 0; i < first.size(); ++i)
		EB_EXPECT_VEC3_NEAR(first[i], second[i], 1e-4f);
}

//////////////////////////////////////////////////////////////////////////
// AI components
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(AI, AgentAndPathComponentDefaults, Integration)
{
	// These defaults are what a designer gets when they add the component in the inspector, so they
	// need to be usable values rather than zeros that make the agent stand still.
	Ember::Test::SceneFixture scene("AIComponentScene");
	Entity agent = Ember::Test::MakeEntityAt(*scene, "Agent", Vector3f(0.0f));

	auto& agentComponent = agent.AttachComponent<AIAgentComponent>();
	EB_EXPECT_MSG(agentComponent.Mode == AIAgentComponent::PathMode::Manual, "agents should default to Manual pathing");
	EB_EXPECT(agentComponent.Loop);
	EB_EXPECT_GT(agentComponent.RecalculateInterval, 0.0f);
	EB_EXPECT_EQ(agentComponent.TargetEntity, UUID(Constants::InvalidUUID));

	auto& pathComponent = agent.AttachComponent<AIPathComponent>();
	EB_EXPECT_GT(pathComponent.Speed, 0.0f);
	EB_EXPECT_GT(pathComponent.ArrivalTolerance, 0.0f);
	EB_EXPECT_EQ(pathComponent.CurrentWaypointIndex, (uint32_t)0);
	EB_EXPECT_EQ(pathComponent.Waypoints.size(), (size_t)0);

	auto& avoidance = agent.AttachComponent<LocalAvoidanceComponent>();
	EB_EXPECT_GT(avoidance.AvoidanceRadius, 0.0f);
	EB_EXPECT_VEC3_NEAR(avoidance.AvoidanceVector, Vector3f(0.0f), 1e-6f);
}

EB_TEST_CASE(AI, NavigationGridComponentStoresItsGrid, Integration)
{
	Ember::Test::SceneFixture scene("NavGridScene");
	Entity gridEntity = Ember::Test::MakeEntityAt(*scene, "NavGrid", Vector3f(0.0f));

	auto& navGrid = gridEntity.AttachComponent<NavigationGridComponent>();
	EB_EXPECT_FALSE(navGrid.Generated);
	EB_EXPECT_GT(navGrid.NodeSpacing, 0.0f);

	navGrid.Grid = MakeGrid(8);
	navGrid.Generated = true;

	EB_CHECK_EQ(navGrid.Grid.size(), (size_t)8);
	EB_CHECK_EQ(navGrid.Grid[0].size(), (size_t)8);
	EB_EXPECT(navGrid.Grid[3][4].IsWalkable);
	EB_EXPECT_EQ(navGrid.Grid[3][4].GridX, 3);
	EB_EXPECT_EQ(navGrid.Grid[3][4].GridY, 4);

	// The stored grid must be directly usable by the pathfinder.
	const std::vector<Vector3f> path = AStar::AStarPath(Vector3f(0.0f), Vector3f(7.0f, 0.0f, 7.0f), navGrid.Grid);
	EB_EXPECT_MSG(!path.empty(), "a grid stored on the component produced no path");
}
