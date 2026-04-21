#include "efpch.h"
#include "SceneHierarchyPanel.h"
#include "Utils/Presets.h"
#include "UI/DragDropTypes.h"

#include "Ember/Scene/Entity.h"
#include <Ember/Event/UIEvent.h>
#include <Ember/Input/Input.h>
#include <Ember/Scene/SceneSerializer.h>
#include <Ember/Core/ProjectManager.h>

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
		EB_DISPATCH_EVENT(MousePressedEvent, OnMousePressed);
	}

	void SceneHierarchyPanel::OnImGuiRender()
	{
		ImGui::Begin(m_Title.c_str());

		ImGui::Separator();

		if (ImGui::Button("Create Entity"))
		{
			CreateEmptyEntity();
		}

		ImGui::Separator();

		RenderEntityTree();
		RenderContextMenu();
		RenderRootParentDragDropZone();

		ImGui::End();
	}

	void SceneHierarchyPanel::RenderContextMenu()
	{
		// Only available in edit mode
		if (m_Context->CurrentSceneState != SceneState::Edit)
			return;

		if (Input::IsKeyPressed(KeyCode::Space) && !ImGui::GetIO().WantTextInput)
		{
			ImGui::OpenPopup("SceneHierarchyContextWindow");
		}

		// Add right click context to pane
		if (ImGui::BeginPopupContextWindow("SceneHierarchyContextWindow", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
		{
			if (ImGui::MenuItem("Create Empty Entity"))
			{
				CreateEmptyEntity();
			}

			if (ImGui::BeginMenu("Create Dynamic"))
			{
				if (ImGui::MenuItem("Character Controller"))
				{
					auto entity = Presets::CreateCharacterController(m_Context->ActiveScene);
					SetSelectedEntity(entity);
					RenameEntity(entity);
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Create Primitive"))
			{
				if (ImGui::MenuItem("Cube"))
				{
					auto entity = Presets::CreateCube(m_Context->ActiveScene);
					CreateEntity(entity);
				}
				if (ImGui::MenuItem("Sphere"))
				{
					auto entity = Presets::CreateSphere(m_Context->ActiveScene);
					CreateEntity(entity);
				}
				if (ImGui::MenuItem("Quad"))
				{
					auto entity = Presets::CreateQuad(m_Context->ActiveScene);
					CreateEntity(entity);
				}
				if (ImGui::MenuItem("Capsule"))
				{
					auto entity = Presets::CreateCapsule(m_Context->ActiveScene);
					CreateEntity(entity);
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Create Light"))
			{
				if (ImGui::MenuItem("Point Light"))
				{
					auto entity = Presets::CreatePointLight(m_Context->ActiveScene);
					SetSelectedEntity(entity);
					RenameEntity(entity);
				}
				if (ImGui::MenuItem("Directional Light"))
				{
					auto entity = Presets::CreateDirectionalLight(m_Context->ActiveScene);
					SetSelectedEntity(entity);
					RenameEntity(entity);
				}
				if (ImGui::MenuItem("Spot Light"))
				{
					auto entity = Presets::CreateSpotLight(m_Context->ActiveScene);
					SetSelectedEntity(entity);
					RenameEntity(entity);
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Create Camera"))
			{
				if (ImGui::MenuItem("3D Camera"))
				{
					auto entity = Presets::Create3DCamera(m_Context->ActiveScene);
					SetSelectedEntity(entity);
					RenameEntity(entity);
				}
				if (ImGui::MenuItem("Camera From View"))
				{
					Quaternion orientation = m_Context->EditorCamera->GetOrientation();
					Vector3f position = m_Context->EditorCamera->GetPosition();
					auto entity = Presets::Create3DCamera(m_Context->ActiveScene, position, orientation);
					SetSelectedEntity(entity);
					RenameEntity(entity);
				}

				ImGui::EndMenu();
			}

			ImGui::EndPopup();
		}
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
			auto& relationshipComp = entity.GetComponent<RelationshipComponent>();
			// Only draw root-level entities here; children are drawn recursively from DrawTreeNode
			if (relationshipComp.ParentHandle == Constants::InvalidUUID)
			{
				DrawTreeNode(entity);
			}
		}

		// De-select if clicking blank space in the hierarchy window
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
		{
			SetSelectedEntity(Entity());
		}

		m_ExpandToSelectedEntity = false;
	}

	void SceneHierarchyPanel::DrawTreeNode(Entity entity)
	{
		ImGuiTreeNodeFlags flags = ((GetSelectedEntity() == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
		flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
		flags |= ImGuiTreeNodeFlags_FramePadding;

		bool hasChildren = !entity.GetComponent<RelationshipComponent>().Children.empty();
		if (!hasChildren)
		{
			flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
		}

		if (m_ExpandToSelectedEntity && IsAncestor(entity, m_Context->SelectedEntity))
		{
			ImGui::SetNextItemOpen(true, ImGuiCond_Always);
		}

		// Render entity tree node item
		bool isDisabled = entity.ContainsComponent<DisabledComponent>();
		if (isDisabled)
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.00f));

		// Renaming state
		bool isRenaming = (m_RenamingEntity == entity);
		auto id = (void*)(uint64_t)(uint32_t)entity.GetEntityHandle();


		bool opened;
		if (isRenaming)
		{
			// If disabled, tint the text darker
			opened = ImGui::TreeNodeEx(id, flags, "");
		}
		else
		{
			// Make prefab instances visually distinct with an orange color
			bool isPrefab = entity.ContainsComponent<PrefabComponent>();
			if (isPrefab)
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.88f, 0.40f, 0.10f, 1.00f));

			opened = ImGui::TreeNodeEx(id, flags, "%s", entity.GetName().c_str());

			if (isPrefab)
				ImGui::PopStyleColor();
		}

		// turn off the disabled tint
		if (isDisabled)
			ImGui::PopStyleColor();

		// Drag drop for parent/child relationships
		{
			std::string payloadType = DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::SceneEntity);
			if (ImGui::BeginDragDropSource())
			{
				UUID payloadUUID = entity.GetUUID();
				ImGui::SetDragDropPayload(payloadType.c_str(), &payloadUUID, sizeof(UUID));
				ImGui::Text("Move %s", entity.GetName().c_str());
				ImGui::EndDragDropSource();
			}

			// Validate the drag payload to prevent circular parent-child relationships
			bool isValidPayload = true;
			if (const ImGuiPayload* payload = ImGui::GetDragDropPayload())
			{
				if (payload->IsDataType(payloadType.c_str()))
				{
					UUID payloadUUID = *(const UUID*)payload->Data;
					bool isSameEntity = payloadUUID == entity.GetUUID();
					bool isDescendant = IsAncestor(m_Context->ActiveScene->GetEntity(payloadUUID), entity);
					bool isParent = entity.GetUUID() == m_Context->ActiveScene->GetEntity(payloadUUID).GetComponent<RelationshipComponent>().ParentHandle;
					// Can't drop onto self, onto a descendant, or onto the current parent
					isValidPayload = !isSameEntity && !isDescendant && !isParent;
				}
			}

			if (isValidPayload)
			{
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payloadType.c_str()))
					{
						UUID payloadUUID = *(const UUID*)payload->Data;
						m_Context->ActiveScene->SetEntityParent(payloadUUID, entity);
					}

					ImGui::EndDragDropTarget();
				}
			}
		}

		// Select if click
		if (ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right))
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
			ImGui::InputText("##Rename", m_RenameBuffer, sizeof(m_RenameBuffer), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
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
				CreateChildEntity(entity);
			}
			if (ImGui::MenuItem("Duplicate Entity", "CTRL+D"))
			{
				DuplicateEntity(entity);
			}
			if (ImGui::MenuItem("Create Prefab From Entity"))
			{
				CreatePrefab(entity);
			}
			if (ImGui::MenuItem("Delete Entity", "DEL"))
			{
				m_Context->PendingEntityRemovals.insert(entity);
			}
			ImGui::Separator();

			bool hasParent = entity.GetComponent<RelationshipComponent>().ParentHandle != Constants::InvalidUUID;
			if (ImGui::MenuItem("Remove Parent", nullptr, false, hasParent))
			{
				auto& relationship = entity.GetComponent<RelationshipComponent>();
				auto parentEntity = m_Context->ActiveScene->GetEntity(relationship.ParentHandle);
				auto& parentRelationship = parentEntity.GetComponent<RelationshipComponent>();
				parentRelationship.Children.erase(std::remove(parentRelationship.Children.begin(), parentRelationship.Children.end(), entity.GetUUID()), parentRelationship.Children.end());
				relationship.ParentHandle = Constants::InvalidUUID;
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

	// Fills remaining space with an invisible drop zone so dragging to blank area removes the parent
	void SceneHierarchyPanel::RenderRootParentDragDropZone()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		ImGui::Dummy(ImGui::GetContentRegionAvail());
		ImGui::PopStyleVar();

		if (ImGui::BeginDragDropTarget())
		{
			std::string payloadType = DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::SceneEntity);
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payloadType.c_str()))
			{
				UUID payloadUUID = *(const UUID*)payload->Data;
				Entity payloadEntity = m_Context->ActiveScene->GetEntity(payloadUUID);
				m_Context->ActiveScene->RemoveParent(payloadEntity);
			}
			ImGui::EndDragDropTarget();
		}
	}

	// Walks up the parent chain from descendant to see if ancestor is one of its parents
	bool SceneHierarchyPanel::IsAncestor(Entity ancestor, Entity descendant)
	{
		if (descendant == Constants::Entities::InvalidEntityID || descendant.GetUUID() == Constants::InvalidUUID)
			return false;

		Entity current = descendant;
		while (current != Constants::Entities::InvalidEntityID)
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

	// BFS through the children of ancestor to check if descendant is below it
	bool SceneHierarchyPanel::IsDescendant(Entity descendant, Entity ancestor)
	{
		if (ancestor == Constants::Entities::InvalidEntityID || ancestor.GetUUID() == Constants::InvalidUUID)
			return false;

		std::vector<UUID> queue;
		queue.push_back(ancestor.GetUUID());
		while (!queue.empty())
		{
			UUID currentUUID = queue[0];
			queue.erase(queue.begin());

			if (currentUUID == descendant.GetUUID())
				return true;

			auto currentEntity = m_Context->ActiveScene->GetEntity(currentUUID);
			auto& children = currentEntity.GetComponent<RelationshipComponent>().Children;
			for (auto& child : children)
				queue.push_back(child);
		}

		return false;
	}

	void SceneHierarchyPanel::CreateEntity(Entity entity)
	{
		SetSelectedEntity(entity);
		RenameEntity(entity);

		auto evt = UINotificationEvent(std::format("Entity {} Created", entity.GetName()));
		m_Context->EventCallback(evt);
	}

	void SceneHierarchyPanel::CreateEmptyEntity()
	{
		auto entity = m_Context->ActiveScene->AddEntity("Empty_Entity");
		CreateEntity(entity);
	}

	void SceneHierarchyPanel::CreateChildEntity(Entity parentEntity)
	{
		auto childEntity = m_Context->ActiveScene->AddEntity("Child_Entity");

		// Set parent to new ChildEntity
		RelationshipComponent& relationship = childEntity.GetComponent<RelationshipComponent>();
		relationship.ParentHandle = parentEntity.GetUUID();
		childEntity.AttachComponent<RelationshipComponent>(relationship);

		// Set relationship on parent to include new ChildEntity
		if (!parentEntity.ContainsComponent<RelationshipComponent>())
		{
			RelationshipComponent parentRelationship;
			parentRelationship.Children.push_back(childEntity.GetUUID());
			parentEntity.AttachComponent<RelationshipComponent>(parentRelationship);
		}
		else
		{
			auto& parentRelationship = parentEntity.GetComponent<RelationshipComponent>();
			parentRelationship.Children.push_back(childEntity.GetUUID());
			parentEntity.AttachComponent<RelationshipComponent>(parentRelationship);
		}

		// Place new child at parent's location
		//EB_CORE_ASSERT(parentEntity.ContainsComponent<TransformComponent>() && childEntity.ContainsComponent<TransformComponent>(), "Entity must contain transform component!");
		//auto& parentTransform = parentEntity.GetComponent<TransformComponent>();
		//auto& childTransform = childEntity.GetComponent<TransformComponent>();
		//childTransform.Position = parentTransform.Position;
		//childTransform.Rotation = parentTransform.Rotation;
		//childTransform.Scale = parentTransform.Scale;
		//childTransform.WorldTransform = parentTransform.WorldTransform;and chil

		SetSelectedEntity(childEntity);
	}

	void SceneHierarchyPanel::DuplicateEntity(Entity entity)
	{
		if (entity == Constants::Entities::InvalidEntityID)
			return;

		auto newEntity = m_Context->ActiveScene->DuplicateEntity(entity);
		SetSelectedEntity(newEntity);
	}

	void SceneHierarchyPanel::CreatePrefab(Entity entity)
	{
		std::string filePath = (ProjectManager::GetActive()->GetAssetDirectory() / "Prefabs" / (entity.GetName() + ".ebprefab")).string();
		SharedPtr<Prefab> prefab = m_Context->ActiveScene->CreatePrefab(entity, filePath);
		if (prefab == nullptr)
		{
			auto evt = UINotificationEvent(std::format("Failed to create prefab from entity {}!", entity.GetName()), UINotificationEvent::Error);
			m_Context->EventCallback(evt);
			return;
		}

		// Success notification
		auto evt = UINotificationEvent(std::format("Prefab {} created!", prefab->GetName()));
		m_Context->EventCallback(evt);
	}

	void SceneHierarchyPanel::RenameEntity(Entity entity)
	{
		m_RenamingEntity = entity;
		strncpy_s(m_RenameBuffer, sizeof(m_RenameBuffer), entity.GetName().c_str(), _TRUNCATE);
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

	bool SceneHierarchyPanel::OnMousePressed(const MousePressedEvent& event)
	{
		auto mb = event.GetMouseButton();
		switch (mb)
		{
			case MouseButton::Left:
				m_SetRenameFocus = false;
				break;
		}

		return false;
	}

}