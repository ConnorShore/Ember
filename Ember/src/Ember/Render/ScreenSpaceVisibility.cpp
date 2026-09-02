#include "ebpch.h"
#include "ScreenSpaceVisibility.h"

#include "Ember/Scene/Scene.h"

namespace Ember {

	namespace {

		// The canvas an element belongs to. A canvas can itself be parented under an organisational
		// entity, so this stops at the first CanvasComponent rather than at the scene root.
		EntityID FindCanvas(Scene* scene, EntityID entity)
		{
			Entity current(entity, scene);
			while (current)
			{
				if (current.ContainsComponent<CanvasComponent>())
					return current.GetEntityHandle();

				if (!current.ContainsComponent<RelationshipComponent>())
					break;

				UUID parentUUID = current.GetComponent<RelationshipComponent>().ParentHandle;
				if (parentUUID == Constants::InvalidUUID)
					break;

				current = scene->GetEntity(parentUUID);
			}

			return (EntityID)Constants::Entities::InvalidEntityID;
		}

		bool IsDescendantOf(Scene* scene, EntityID entity, EntityID ancestor)
		{
			Entity current(entity, scene);
			while (current && current.ContainsComponent<RelationshipComponent>())
			{
				UUID parentUUID = current.GetComponent<RelationshipComponent>().ParentHandle;
				if (parentUUID == Constants::InvalidUUID)
					return false;

				current = scene->GetEntity(parentUUID);
				if (current.GetEntityHandle() == ancestor)
					return true;
			}

			return false;
		}

	}

	ScreenSpaceVisibility::ScreenSpaceVisibility(Scene* scene, ScreenSpaceRenderMode mode, EntityID selectedEntity)
		: m_Scene(scene), m_Mode(mode)
	{
		Entity selected(selectedEntity, scene);
		if (!selected)
			return;

		m_Selected = selectedEntity;
		m_SelectedCanvas = FindCanvas(scene, selectedEntity);

		// A selection outside any canvas has no UI ancestors, so there is nothing to collect.
		if (m_SelectedCanvas == (EntityID)Constants::Entities::InvalidEntityID)
			return;

		// The canvas caps the walk: it never enters the draw list, and neither does anything above it.
		Entity current = selected;
		while (current && current.GetEntityHandle() != m_SelectedCanvas && current.ContainsComponent<RelationshipComponent>())
		{
			UUID parentUUID = current.GetComponent<RelationshipComponent>().ParentHandle;
			if (parentUUID == Constants::InvalidUUID)
				break;

			current = scene->GetEntity(parentUUID);
			if (!current || current.GetEntityHandle() == m_SelectedCanvas)
				break;

			m_SelectedAncestors.push_back(current.GetEntityHandle());
		}
	}

	bool ScreenSpaceVisibility::ShouldRender(EntityID entity) const
	{
		if (m_Mode == ScreenSpaceRenderMode::All)
			return true;

		if (m_Mode == ScreenSpaceRenderMode::None)
			return false;

		// With nothing selected there is no reference point for the selection-relative bits, so
		// every canvas counts as one the selection does not belong to.
		if (m_Selected == (EntityID)Constants::Entities::InvalidEntityID)
			return HasFlag(m_Mode, ScreenSpaceRenderMode::OtherCanvases);

		if (entity == m_Selected)
			return HasFlag(m_Mode, ScreenSpaceRenderMode::Selected);

		if (std::find(m_SelectedAncestors.begin(), m_SelectedAncestors.end(), entity) != m_SelectedAncestors.end())
			return HasFlag(m_Mode, ScreenSpaceRenderMode::Parents);

		if (IsDescendantOf(m_Scene, entity, m_Selected))
			return HasFlag(m_Mode, ScreenSpaceRenderMode::Children);

		// The groups are disjoint, so Canvas covers only what the three tests above did not claim.
		if (FindCanvas(m_Scene, entity) == m_SelectedCanvas)
			return HasFlag(m_Mode, ScreenSpaceRenderMode::Canvas);

		return HasFlag(m_Mode, ScreenSpaceRenderMode::OtherCanvases);
	}

}
