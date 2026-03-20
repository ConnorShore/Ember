#pragma once

#include "Panel.h"
#include <Ember.h>

namespace Ember {

	class SceneHierarchyPanel : public Panel
	{
	public:
		SceneHierarchyPanel();
		virtual ~SceneHierarchyPanel();

		void OnEvent(Event& event) override;
		void OnImGuiRender() override;

		inline void SetSelectedEntity(Entity entity) { m_Context->SelectedEntity = entity; m_PreviouslySelectedEntity = entity; }
		inline Entity GetSelectedEntity() const { return m_Context->SelectedEntity; }

	private:
		void RenderEntityTree();
		void DrawTreeNode(Entity entity);

		bool IsAncestor(Entity ancestor, Entity descendant);

		void CreateEntity();

	private:
		Entity m_PreviouslySelectedEntity;
		bool m_ExpandToSelectedEntity = true;
	};
}