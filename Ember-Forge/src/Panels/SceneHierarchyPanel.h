#pragma once

#include "Panel.h"
#include <Ember.h>

namespace Ember {

	class SceneHierarchyPanel : public Panel
	{
	public:
		SceneHierarchyPanel(EditorContext* context);
		virtual ~SceneHierarchyPanel();

		void OnEvent(Event& event) override;
		void OnImGuiRender() override;

		inline void SetSelectedEntity(Entity entity) { m_Context->SelectedEntity = entity; m_PreviouslySelectedEntity = entity; }
		inline Entity GetSelectedEntity() const { return m_Context->SelectedEntity; }

	private:
		void RenderContextMenu();
		void RenderEntityTree();
		void DrawTreeNode(Entity entity);

		bool IsAncestor(Entity ancestor, Entity descendant);

		void CreateEmptyEntity();
		void DuplicateEntity(Entity entity);
		void RenameEntity(Entity entity);

		bool OnKeyPressed(const KeyPressedEvent& event);
		bool OnMousePressed(const MousePressedEvent& event);

	private:
		Entity m_PreviouslySelectedEntity;
		bool m_ExpandToSelectedEntity = true;

		Entity m_RenamingEntity;
		char m_RenameBuffer[256] = "";
		bool m_SetRenameFocus = false;
	};
}