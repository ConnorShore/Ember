#pragma once

#include "ScreenSpaceRenderMode.h"

#include "Ember/Core/Constants.h"
#include "Ember/ECS/Types.h"

#include <vector>

namespace Ember {

	class Scene;

	// Resolves a ScreenSpaceRenderMode against one selection. Every screen-space element falls into
	// exactly one group - the selection, its children, its parents, its canvas, or another canvas -
	// so each bit is an independent checkbox rather than a widening level.
	class ScreenSpaceVisibility
	{
	public:
		ScreenSpaceVisibility(Scene* scene, ScreenSpaceRenderMode mode, EntityID selectedEntity);

		bool ShouldRender(EntityID entity) const;

		// The canvas the selection sits under, or an invalid handle when it is not UI at all.
		EntityID GetSelectedCanvas() const { return m_SelectedCanvas; }

	private:
		Scene* m_Scene = nullptr;
		ScreenSpaceRenderMode m_Mode = ScreenSpaceRenderMode::All;

		EntityID m_Selected = (EntityID)Constants::Entities::InvalidEntityID;
		EntityID m_SelectedCanvas = (EntityID)Constants::Entities::InvalidEntityID;

		// Walked once in the constructor so ShouldRender stays a lookup rather than a hierarchy climb.
		std::vector<EntityID> m_SelectedAncestors;
	};

}
