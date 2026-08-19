#pragma once

#include "System.h"
#include "Ember/ECS/Types.h"
#include "Ember/ECS/Registry.h"
#include "Ember/Math/Math.h"

namespace Ember {

	class Scene;

	// One screen-space UI element in draw order: canvas sort order first, then depth-first hierarchy.
	// Entities without a RectTransform are included - they inherit their parent's rect and still render.
	struct UIDrawEntry
	{
		EntityID Entity = (EntityID)Constants::Entities::InvalidEntityID;
		uint32_t CanvasSortOrder = 0;
		uint32_t HierarchyIndex = 0;
	};

	class UILayoutSystem : public System
	{
	public:
		UILayoutSystem() = default;
		virtual ~UILayoutSystem() = default;

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate(TimeStep delta, Scene* scene) override;
		void OnViewportResize(Scene* scene, uint32_t width, uint32_t height);

		// Rebuilt every frame by OnUpdate; the single source of truth for UI draw and hit-test order.
		const std::vector<UIDrawEntry>& GetSortedScreenSpaceEntities() const { return m_SortedEntities; }

	private:
		std::vector<UIDrawEntry> m_SortedEntities;
	};

}