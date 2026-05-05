#include "ebpch.h"
#include "AISystem.h"

#include "Ember/AI/NavigationGrid.h"
#include "Ember/Render/DebugRenderer.h"


namespace Ember {

	// Define some colors
	constexpr Vector4f UnselectedColor(0.5f, 0.0f, 0.5f, 0.5f); // Semi-transparent purple for unselected paths
	constexpr Vector4f SelectedColor(1.0f, 0.0f, 1.0f, 1.0f);   // Magenta for highlighted paths
	constexpr Vector4f WaypointColor(0.0f, 1.0f, 1.0f, 1.0f);   // Cyan for individual waypoints

	// Normalize a segment pair so {A,B} and {B,A} map to the same key
	auto MakeSegmentKey = [](UUID a, UUID b) -> std::pair<UUID, UUID> {
		return (a < b) ? std::make_pair(a, b) : std::make_pair(b, a);
	};

	AISystem::AISystem()
	{

	}

	AISystem::~AISystem()
	{

	}

	void AISystem::OnAttach()
	{

	}

	void AISystem::OnUpdate(TimeStep delta, Scene* scene)
	{
		auto& registry = scene->GetRegistry();
		auto view = registry.ActiveQuery<AIAgentComponent, AIPathComponent>();
		for (EntityID e : view)
		{
			auto& agentComp = registry.GetComponent<AIAgentComponent>(e);
			if (agentComp.Dirty)
				ApplyAgentModeSettings(Entity(e, scene), scene);

			if (agentComp.Mode == AIAgentComponent::PathMode::Dynamic)
			{
				// Run A* when the interval timer elapses
			}
			else if (agentComp.Mode == AIAgentComponent::PathMode::Manual)
			{
				// Nothing to update for manual paths
			}
			else
			{
				EB_CORE_ASSERT(false, "Unhandled AIAgent PathMode!");
			}
		}
	}

	void AISystem::OnDetach()
	{

	}

	void AISystem::OnSceneAttach(Scene* scene)
	{
		auto& registry = scene->GetRegistry();
		
		// Set agent mode settings when the scene begins
		auto view = registry.ActiveQuery<AIAgentComponent>();
		for (EntityID e : view)
			ApplyAgentModeSettings(Entity(e, scene), scene);
	}

	void AISystem::OnSceneDetach(Scene* scene)
	{

	}

	void AISystem::OnEditorUpdate(TimeStep delta, Scene* scene)
	{
		auto& registry = scene->GetRegistry();

		// We will track which paths should be highlighted
		std::vector<EntityID> pathsToHighlight = RenderPreviewEntityPaths(scene);

		RenderAllPathsDebug(scene, pathsToHighlight);
		RenderNavigationGridsDebug(scene);
	}

	void AISystem::ApplyAgentModeSettings(Entity agentEntity, Scene* scene)
	{
		EB_CORE_ASSERT(agentEntity.ContainsComponent<AIAgentComponent>() && agentEntity.ContainsComponent<AIPathComponent>(), 
			"Entity must have both AIAgentComponent and AIPathComponent");

		auto& agentComp = agentEntity.GetComponent<AIAgentComponent>();
		auto& pathComp = agentEntity.GetComponent<AIPathComponent>();

		if (agentComp.Mode == AIAgentComponent::PathMode::Dynamic)
		{
			// Clear manual waypoints and path since we're switching to dynamic
			pathComp.Waypoints.clear();
			return;
		}

		if (agentComp.Mode == AIAgentComponent::PathMode::Manual)
		{
			// When switching to manual, we could either keep the existing path as-is or clear it. Here we choose to clear it.
			pathComp.Waypoints.clear();
			pathComp.Waypoints.reserve(agentComp.ManualWaypoints.size());

			for (UUID wpUUID : agentComp.ManualWaypoints)
			{
				if (wpUUID == Constants::InvalidUUID)
					continue;
				Entity wpEntity = scene->GetEntity(wpUUID);
				if (wpEntity.GetUUID() == Constants::InvalidUUID)
					continue;
				if (!wpEntity.ContainsComponent<TransformComponent>())
					continue;
				auto& wpTransform = wpEntity.GetComponent<TransformComponent>();
				pathComp.Waypoints.push_back(wpTransform.GetWorldTransform()[3]);
			}
			return;
		}

		EB_CORE_ASSERT(false, "Unhandled AIAgent PathMode!");
	}

	std::vector<EntityID> AISystem::RenderPreviewEntityPaths(Scene* scene)
	{
		if (m_PreviewEntity == Constants::Entities::InvalidEntityID)
			return {};

		auto& registry = scene->GetRegistry();
		std::vector<EntityID> pathsToHighlight;

		// Determine what is currently selected
		Entity selectedEntity(m_PreviewEntity, scene);

		// If the AI itself is selected, flag its path to be highlighted
		if (selectedEntity.ContainsComponent<AIAgentComponent>())
			pathsToHighlight.push_back(selectedEntity.GetEntityHandle());

		// If a Waypoint is selected, find EVERY path that uses this waypoint and highlight them!
		if (selectedEntity.ContainsComponent<WaypointComponent>())
		{
			// Draw a sphere around the selected waypoint
			auto& wpTransform = selectedEntity.GetComponent<TransformComponent>();
			auto& wpComponent = selectedEntity.GetComponent<WaypointComponent>();
			// Assuming you have a DrawSphere or DrawBox in your DebugRenderer
			DebugRenderer::DrawOctahedron(wpTransform.GetWorldTransform()[3], 0.5f, WaypointColor);

			if (wpComponent.ShowPaths)
			{
				auto view = registry.ActiveQuery<AIAgentComponent>();
				for (EntityID e : view)
				{
					auto& agentComp = registry.GetComponent<AIAgentComponent>(e);
					// If this path contains the selected waypoint, highlight the whole path
					if (std::find(agentComp.ManualWaypoints.begin(), agentComp.ManualWaypoints.end(), selectedEntity.GetUUID()) != agentComp.ManualWaypoints.end())
						pathsToHighlight.push_back(e);
				}
			}
		}

		return pathsToHighlight;
	}

	void AISystem::RenderNavigationGridsDebug(Scene* scene)
	{
		auto& registry = scene->GetRegistry();

		bool previewEntityIsSelected = false;
		if (m_PreviewEntity != Constants::Entities::InvalidEntityID)
		{
			Entity previewEnt(m_PreviewEntity, scene);
			if (previewEnt.ContainsComponent<NavigationGridComponent>())
				previewEntityIsSelected = true;
		}

		if (m_DebugRenderSettings.Enabled)
		{
			auto view = registry.ActiveQuery<NavigationGridComponent, TransformComponent>();
			for (EntityID e : view)
			{
				auto [navGrid, transform] = registry.GetComponents<NavigationGridComponent, TransformComponent>(e);

				if (navGrid.Generated)
					NavigationGrid::RenderGeneratedGrid(navGrid.Grid, previewEntityIsSelected);
				else
					NavigationGrid::RenderUngeneratedGrid(transform.WorldTransform[3], transform.Scale.x, transform.Scale.z, previewEntityIsSelected);
			}
		}
		else if (previewEntityIsSelected)
		{
			auto [navGrid, transform] = registry.GetComponents<NavigationGridComponent, TransformComponent>(m_PreviewEntity);

			if (navGrid.Generated)
				NavigationGrid::RenderGeneratedGrid(navGrid.Grid, true);
			else
				NavigationGrid::RenderUngeneratedGrid(transform.WorldTransform[3], transform.Scale.x, transform.Scale.z, true);
		}
	}

	void AISystem::RenderAllPathsDebug(Scene* scene, const std::vector<EntityID>& pathsToHighlight)
	{
		auto& registry = scene->GetRegistry();

		// Normalize a segment pair so {A,B} and {B,A} map to the same key
		auto makeSegmentKey = [](UUID a, UUID b) -> std::pair<UUID, UUID> {
			return (a < b) ? std::make_pair(a, b) : std::make_pair(b, a);
		};

		// Pre-collect all segments belonging to highlighted paths so they can be drawn last (on top)
		std::vector<HighlightedSegment> highlightedSegments;
		std::unordered_set<std::pair<UUID, UUID>, PairUUIDHash> highlightedSegmentKeys;
		CalculateHighlightedSegments(scene, pathsToHighlight, highlightedSegments, highlightedSegmentKeys);

		// Pass 1: Draw unhighlighted segments, skipping any that overlap a highlighted segment (only if debug rendering is enabled)
		RenderUnhighlightedSegments(scene, pathsToHighlight, highlightedSegmentKeys);

		// Pass 2: Draw highlighted segments and their waypoint markers last, so they appear on top
		// We draw these all the time even if debug rendering is disabled, since they represent the selected path(s)
		RenderHighlightedSegments(scene, highlightedSegments);
	}

	// TODO: See if can minimize code duplication below by merging with RenderUnhighlightedSegments
	void AISystem::CalculateHighlightedSegments(Scene* scene, const std::vector<EntityID>& pathsToHighlight, std::vector<HighlightedSegment>& highlightedSegments, std::unordered_set<std::pair<UUID, UUID>, PairUUIDHash>& highlightedSegmentKeys)
	{
		auto& registry = scene->GetRegistry();

		auto view = registry.ActiveQuery<AIAgentComponent>();
		for (EntityID e : view)
		{
			auto& agentComp = registry.GetComponent<AIAgentComponent>(e);
			if (agentComp.Mode != AIAgentComponent::PathMode::Manual)
				continue;

			if (agentComp.ManualWaypoints.size() < 2)
				continue;

			if (std::find(pathsToHighlight.begin(), pathsToHighlight.end(), e) == pathsToHighlight.end())
				continue;

			for (size_t i = 0; i < agentComp.ManualWaypoints.size(); i++)
			{
				UUID currentWP = agentComp.ManualWaypoints[i];
				UUID nextWP = Constants::InvalidUUID;

				if (i < agentComp.ManualWaypoints.size() - 1)
					nextWP = agentComp.ManualWaypoints[i + 1];
				else if (agentComp.Loop)
					nextWP = agentComp.ManualWaypoints[0];

				if (currentWP == Constants::InvalidUUID || nextWP == Constants::InvalidUUID)
					continue;

				Entity currentWPEntity = scene->GetEntity(currentWP);
				Entity nextWPEntity = scene->GetEntity(nextWP);
				if (!currentWPEntity.ContainsComponent<TransformComponent>() || !nextWPEntity.ContainsComponent<TransformComponent>())
					continue;

				Vector3f startPos = currentWPEntity.GetComponent<TransformComponent>().WorldTransform[3];
				Vector3f endPos = nextWPEntity.GetComponent<TransformComponent>().WorldTransform[3];

				auto key = MakeSegmentKey(currentWP, nextWP);
				if (highlightedSegmentKeys.insert(key).second)
					highlightedSegments.push_back({ startPos, endPos });
			}
		}
	}

	void AISystem::RenderUnhighlightedSegments(Scene* scene, const std::vector<EntityID>& pathsToHighlight, const std::unordered_set<std::pair<UUID, UUID>, PairUUIDHash>& highlightedSegmentKeys)
	{
		if (!m_DebugRenderSettings.Enabled)
			return;

		auto& registry = scene->GetRegistry();

		std::unordered_set<std::pair<UUID, UUID>, PairUUIDHash> drawnSegments;
		auto view = registry.ActiveQuery<AIAgentComponent>();
		for (EntityID e : view)
		{
			auto& agentComp = registry.GetComponent<AIAgentComponent>(e);
			if (agentComp.Mode != AIAgentComponent::PathMode::Manual)
				continue;

			if (agentComp.ManualWaypoints.size() < 2)
				continue;

			if (std::find(pathsToHighlight.begin(), pathsToHighlight.end(), e) != pathsToHighlight.end())
				continue;

			for (size_t i = 0; i < agentComp.ManualWaypoints.size(); i++)
			{
				UUID currentWP = agentComp.ManualWaypoints[i];
				UUID nextWP = Constants::InvalidUUID;

				if (i < agentComp.ManualWaypoints.size() - 1)
					nextWP = agentComp.ManualWaypoints[i + 1];
				else if (agentComp.Loop)
					nextWP = agentComp.ManualWaypoints[0];

				if (currentWP == Constants::InvalidUUID || nextWP == Constants::InvalidUUID)
					continue;

				auto key = MakeSegmentKey(currentWP, nextWP);

				// Skip if already drawn or if a highlighted path owns this segment
				if (!drawnSegments.insert(key).second || highlightedSegmentKeys.count(key))
					continue;

				Entity currentWPEntity = scene->GetEntity(currentWP);
				Entity nextWPEntity = scene->GetEntity(nextWP);
				if (!currentWPEntity.ContainsComponent<TransformComponent>() || !nextWPEntity.ContainsComponent<TransformComponent>())
					continue;

				Vector3f startPos = currentWPEntity.GetComponent<TransformComponent>().WorldTransform[3];
				Vector3f endPos = nextWPEntity.GetComponent<TransformComponent>().WorldTransform[3];
				DebugRenderer::DrawLine(startPos, endPos, UnselectedColor);
			}
		}
	}

	void AISystem::RenderHighlightedSegments(Scene* scene, const std::vector<HighlightedSegment>& highlightedSegments)
	{
		for (uint32_t i = 0; i < highlightedSegments.size(); i++)
		{
			auto& seg = highlightedSegments[i];
			DebugRenderer::DrawLine(seg.start, seg.end, SelectedColor);
			DebugRenderer::DrawOctahedron(seg.start, 0.5f, SelectedColor);

			// Draw end node if last node
			if (i == highlightedSegments.size() - 1)
				DebugRenderer::DrawOctahedron(seg.end, 0.5f, SelectedColor);
		}
	}

}