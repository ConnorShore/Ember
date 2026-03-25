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
		EB_CREATE_DISPATCHER(event);
		EB_DISPATCH_EVENT(KeyPressedEvent, OnKeyPressed);
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
			if (relationshipComp.ParentHandle == Constants::InvalidUUID)
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
		flags |= ImGuiTreeNodeFlags_SpanAvailWidth;

		bool hasChildren = !entity.GetComponent<RelationshipComponent>().Children.empty();
		if (!hasChildren)
		{
			flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
		}

		if (m_ExpandToSelectedEntity && IsAncestor(entity, m_Context->SelectedEntity))
		{
			ImGui::SetNextItemOpen(true, ImGuiCond_Always);
		}

		// Renaming state
		bool isRenaming = (m_RenamingEntity == entity);
		auto id = (void*)(uint64_t)(uint32_t)entity.GetEntityHandle();

		bool opened;
		if (isRenaming)
		{
			opened = ImGui::TreeNodeEx(id, flags, "");
		}
		else
		{
			opened = ImGui::TreeNodeEx(id, flags, "%s", entity.GetName().c_str());
		}

		if (ImGui::IsItemClicked())
		{
			SetSelectedEntity(entity);
		}

		// Double-Click Rename Trigger
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			RenameEntity(entity);
		}

		if (isRenaming)
		{
			ImGui::SameLine();

			if (m_SetRenameFocus)
			{
				ImGui::SetKeyboardFocusHere();
				m_SetRenameFocus = false;
			}

			ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);

			// If the user hits Enter, apply the name
			if (ImGui::InputText("##Rename", m_RenameBuffer, sizeof(m_RenameBuffer), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
			{
				entity.GetComponent<TagComponent>().Tag = std::string(m_RenameBuffer);
				m_RenamingEntity = {};
			}
			ImGui::PopItemWidth();

			if (ImGui::IsItemDeactivated())
			{
				if (ImGui::IsKeyPressed(ImGuiKey_Escape))
				{
					m_RenamingEntity = {};
				}
				else
				{
					entity.GetComponent<TagComponent>().Tag = std::string(m_RenameBuffer); // Apply
					m_RenamingEntity = {};
				}
			}
		}

		if (m_ExpandToSelectedEntity && m_Context->SelectedEntity == entity)
		{
			ImGui::SetScrollHereY(0.5f);
		}

		std::string popupId = entity.GetName() + "##" + std::to_string((uint32_t)entity.GetEntityHandle());
		if (ImGui::BeginPopupContextItem(popupId.c_str(), ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
		{
			if (ImGui::MenuItem("Rename Entity", "F2"))
			{
				RenameEntity(entity);
			}
			if (ImGui::MenuItem("Add Child Entity"))
			{
				// TODO: Add this functionality
			}
			if (ImGui::MenuItem("Duplicate Entity", "CTRL+D"))
			{
				DuplicateEntity(entity);
			}
			if (ImGui::MenuItem("Delete Entity", "DEL"))
			{
				m_Context->PendingEntityRemovals.insert(entity);
			}

			ImGui::EndPopup();
		}

		if (opened)
		{
			if (hasChildren)
			{
				auto& children = entity.GetComponent<RelationshipComponent>().Children;
				for (UUID childID : children)
				{
					Entity child = m_Context->ActiveScene->GetEntity(childID);
					if (child != Constants::Entities::InvalidEntityID)
						DrawTreeNode(child);
				}
				ImGui::TreePop();
			}
		}
	}

	bool SceneHierarchyPanel::IsAncestor(Entity ancestor, Entity descendant)
	{
		if (descendant == Constants::Entities::InvalidEntityID || descendant.GetUUID() == Constants::InvalidUUID)
			return false;

		Entity current = descendant;
		while (current != Constants::Entities::InvalidComponentID)
		{
			UUID parentID = current.GetComponent<RelationshipComponent>().ParentHandle;
			if (parentID == Constants::InvalidUUID)
				break;

			if (parentID == ancestor.GetUUID())
				return true;

			current = m_Context->ActiveScene->GetEntity(parentID);
		}

		return false;
	}

	void SceneHierarchyPanel::CreateEntity()
	{
		auto entity = m_Context->ActiveScene->AddEntity("Empty_Entity");
		SetSelectedEntity(entity);
		RenameEntity(entity);
	}

	void SceneHierarchyPanel::DuplicateEntity(Entity entity)
	{
		if (entity == Constants::Entities::InvalidEntityID)
			return;

		auto newEntity = m_Context->ActiveScene->DuplicateEntity(entity);
		SetSelectedEntity(newEntity);
	}

	void SceneHierarchyPanel::RenameEntity(Entity entity)
	{
		m_RenamingEntity = entity;
		strncpy(m_RenameBuffer, "Empty_Entity", sizeof(m_RenameBuffer));
		m_SetRenameFocus = true;
	}

	bool SceneHierarchyPanel::OnKeyPressed(const KeyPressedEvent& e)
	{
		if (ImGui::GetIO().WantTextInput)
			return false;

		bool control = Input::IsKeyPressed(KeyCode::LeftControl) || Input::IsKeyPressed(KeyCode::RightControl);
		bool shift = Input::IsKeyPressed(KeyCode::LeftShift) || Input::IsKeyPressed(KeyCode::RightShift);

		KeyCode key = e.GetKeyCode();
		switch (key)
		{
		case KeyCode::D:
			if (control) DuplicateEntity(m_Context->SelectedEntity);
			break;
		case KeyCode::F2:
			RenameEntity(m_Context->SelectedEntity);
			break;
		case KeyCode::Delete:
			if (m_Context->SelectedEntity != Constants::Entities::InvalidEntityID)
				m_Context->PendingEntityRemovals.insert(m_Context->SelectedEntity);
			break;
		}

		return false;
	}

}