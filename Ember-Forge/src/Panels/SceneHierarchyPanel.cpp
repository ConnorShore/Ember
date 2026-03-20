#include "SceneHierarchyPanel.h"
#include "Ember/Scene/Entity.h"

namespace Ember {

	SceneHierarchyPanel::SceneHierarchyPanel(EditorContext* context)
		: Panel("Scene Hierarchy", context)
	{
	}

	SceneHierarchyPanel::~SceneHierarchyPanel()
	{
	}

	void SceneHierarchyPanel::OnEvent(Event& event)
	{
	}

	void SceneHierarchyPanel::OnImGuiRender()
	{
		ImGui::Begin(m_Title.c_str());

		ImGui::Separator();

		if (ImGui::Button("Create Entity"))
			CreateEntity();

		ImGui::Separator();

		RenderEntityTree();


		// Add right click context to pane
		if (ImGui::BeginPopupContextWindow("SceneHierarchyContextWindow", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
		{
			if (ImGui::MenuItem("Create Empty Entity"))
			{
				CreateEntity();
			}

			ImGui::EndPopup();
		}

		ImGui::End();
	}

	void SceneHierarchyPanel::RenderEntityTree()
	{
		if (!m_Context->ActiveScene)
			return;

		auto entities = m_Context->ActiveScene->GetAllEntities();

		if (m_Context->SelectedEntity != m_PreviouslySelectedEntity)
		{
			m_PreviouslySelectedEntity = m_Context->SelectedEntity;
			m_ExpandToSelectedEntity = true;
		}

        for (auto& entity : entities) 
		{
			// Draw the tree node
			auto& relationshipComp = entity.GetComponent<RelationshipComponent>();
			if (relationshipComp.ParentHandle == Constants::Entities::InvalidEntityID)
			{
				DrawTreeNode(entity);
			}
		}

		// De-select if clicking blank space in the hierarchy window
		if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
		{
			SetSelectedEntity(Entity());
		}

		m_ExpandToSelectedEntity = false;
	}

    void SceneHierarchyPanel::DrawTreeNode(Entity entity)
	{
		ImGuiTreeNodeFlags flags = ((GetSelectedEntity() == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
		flags |= ImGuiTreeNodeFlags_SpanAvailWidth; // Makes the selection highlight span the whole window width

		bool hasChildren = !entity.GetComponent<RelationshipComponent>().Children.empty();
		if (!hasChildren)
		{
			flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
		}

		if (m_ExpandToSelectedEntity)
		{
			if (IsAncestor(entity, m_Context->SelectedEntity))
			{
				ImGui::SetNextItemOpen(true, ImGuiCond_Always);
			}
		}

		auto id = (void*)(uint64_t)(uint32_t)entity.GetEntityHandle();
		bool opened = ImGui::TreeNodeEx(id, flags, "%s", entity.GetName().c_str());
		if (ImGui::IsItemClicked())
		{
			SetSelectedEntity(entity);
		}

		if (m_ExpandToSelectedEntity && m_Context->SelectedEntity == entity)
		{
			ImGui::SetScrollHereY(0.5f); // 0.5f centers the item vertically in the window
		}

		// Entity context menu
		if (ImGui::BeginPopupContextItem(entity.GetName().c_str(), ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
		{
			if (ImGui::MenuItem("Rename Entity"))
			{
				// TODO: Add this functionality
			}
			if (ImGui::MenuItem("Add Child Entity"))
			{
				// TODO: Add this functionality
			}
			if (ImGui::MenuItem("Duplicate Entity"))
			{
				// TODO: Add this functionality
			}
			if (ImGui::MenuItem("Delete Entity"))
			{
				m_Context->PendingEntityRemovals.insert(entity);
			}

			ImGui::EndPopup();
		}

		if (opened && hasChildren)
		{
			auto& children = entity.GetComponent<RelationshipComponent>().Children;
			for (auto childID : children)
			{
				Entity child(childID, m_Context->ActiveScene.Ptr());
				DrawTreeNode(child);
			}

			// We only pop the tree if it's NOT a leaf node
			ImGui::TreePop();
		}
	}

	bool SceneHierarchyPanel::IsAncestor(Entity ancestor, Entity descendant)
	{
		if (!descendant || descendant.GetEntityHandle() == Constants::Entities::InvalidEntityID)
			return false;

		Entity current = descendant;
		while (current != Constants::Entities::InvalidComponentID)
		{
			EntityID parentID = current.GetComponent<RelationshipComponent>().ParentHandle;
			if (parentID == Constants::Entities::InvalidEntityID)
				break;

			if (parentID == ancestor.GetEntityHandle())
				return true;

			current = Entity{ parentID, m_Context->ActiveScene.Ptr()};
		}

		return false;
	}

	void SceneHierarchyPanel::CreateEntity()
	{
		auto entity = m_Context->ActiveScene->AddEntity();
		SetSelectedEntity(entity);
	}

}