#pragma once

#include "System.h"
#include "Ember/Scene/Entity.h"

#include <vector>

namespace Ember {

	struct AIDebugRenderSettings
	{
		bool Enabled = true;
	};

	class AISystem : public System
	{
	public:
		AISystem();
		virtual ~AISystem();

		void OnAttach() override;
		void OnUpdate(TimeStep delta, Scene* scene) override;
		void OnDetach() override;

		void OnSceneAttach(Scene* scene) override;
		void OnSceneDetach(Scene* scene) override;

		void OnEditorUpdate(TimeStep delta, Scene* scene);

		void SetPreviewEntity(EntityID entityID) { m_PreviewEntity = entityID; }
		void ClearPreviewEntity() { m_PreviewEntity = Constants::Entities::InvalidEntityID; }

		void ApplyAgentModeSettings(Entity agentEntity, Scene* scene);

		AIDebugRenderSettings& GetDebugRenderSettings() { return m_DebugRenderSettings; }

	private:
		// TODO: Split AIDebugRender stuff to its own class eventually and reserve this system for ai logic only
		//  Should do this with physics system as well at some point
		struct HighlightedSegment 
		{ 
			Vector3f start, end; 
		};

		struct PairUUIDHash {
			std::size_t operator()(const std::pair<UUID, UUID>& p) const {
				std::size_t h1 = std::hash<UUID>{}(p.first);
				std::size_t h2 = std::hash<UUID>{}(p.second);
				return h1 ^ (h2 << 32) ^ (h2 >> 32);
			}
		};

	private:
		std::vector<EntityID> RenderPreviewEntityPaths(Scene* scene);

		void RenderNavigationGridsDebug(Scene* scene);
		void RenderAllPathsDebug(Scene* scene, const std::vector<EntityID>& pathsToHighlight);
		void CalculateHighlightedSegments(Scene* scene, const std::vector<EntityID>& pathsToHighlight, std::vector<HighlightedSegment>& highlightedSegments, std::unordered_set<std::pair<UUID, UUID>, PairUUIDHash>& highlightedSegmentKeys);
		void RenderUnhighlightedSegments(Scene* scene, const std::vector<EntityID>& pathsToHighlight, const std::unordered_set<std::pair<UUID, UUID>, PairUUIDHash>& highlightedSegmentKeys);
		void RenderHighlightedSegments(Scene* scene, const std::vector<HighlightedSegment>& highlightedSegments);

	private:
		AIDebugRenderSettings m_DebugRenderSettings;
		EntityID m_PreviewEntity = Constants::Entities::InvalidEntityID;
	};

}