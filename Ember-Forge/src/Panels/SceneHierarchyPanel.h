#pragma once

#include "Panel.h"

namespace Ember {

	class SceneHierarchyPanel : public Panel
	{
	public:
		SceneHierarchyPanel(EditorContext* context);
		virtual ~SceneHierarchyPanel();

		void OnEvent(Event& event) override;
		void OnImGuiRender() override;

		void SetSelectedEntity(Entity entity);
		inline Entity GetSelectedEntity() const { return m_Context->SelectedEntity; }

	private:
		void RenderContextMenu();
		void RenderEntityTree();
		void DrawTreeNode(Entity entity);
		void HandlePrefabDragDrop(Entity entity);
		void HandleEntityDragDrop(Entity entity);
		void RenderRootParentDragDropZone();

		bool CanDropEntityAsChild(UUID payloadUUID, Entity targetParent);
		bool CanDropEntityAsSibling(UUID payloadUUID, Entity targetSibling);
		bool IsAncestor(Entity ancestor, Entity descendant);
		bool IsDescendant(Entity descendant, Entity ancestor);

		void CreateEntity(Entity entity);
		void PlaceNewEntityAtSpawnPoint(Entity entity);
		void CreateEmptyEntity();
		void CreateChildEntity(Entity parentEntity);
		void DuplicateEntity(Entity entity);
		void DuplicateSelection();
		void RemoveSelection();
		void SelectAllEntities();
		void SelectRangeTo(Entity entity);
		std::vector<Entity> SelectionRoots() const;
		void CreatePrefab(Entity entity);
		void RenameEntity(Entity entity);

		bool OnKeyPressed(const KeyPressedEvent& event);
		bool OnMousePressed(const MousePressedEvent& event);

	private:
		Entity m_PreviouslySelectedEntity;
		bool m_ExpandToSelectedEntity = true;

		// Flattened draw order from last frame, and where a Shift+click range starts from.
		std::vector<Entity> m_VisibleOrder;
		Entity m_SelectionAnchor;

		Entity m_RenamingEntity;
		char m_RenameBuffer[256] = "";
		bool m_SetRenameFocus = false;
	};
}