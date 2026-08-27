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
	namespace {
		enum class HierarchyDropPlacement
		{
			Before,
			Into,
			After
		};

		HierarchyDropPlacement GetHierarchyDropPlacement()
		{
			float itemHeight = ImGui::GetItemRectSize().y;
			if (itemHeight <= 0.0f)
				return HierarchyDropPlacement::Into;

			float localMouseY = ImGui::GetMousePos().y - ImGui::GetItemRectMin().y;
			float reorderZoneHeight = itemHeight * 0.30f;
			if (localMouseY <= reorderZoneHeight)
				return HierarchyDropPlacement::Before;

			if (localMouseY >= itemHeight - reorderZoneHeight)
				return HierarchyDropPlacement::After;

			return HierarchyDropPlacement::Into;
		}

		void DrawHierarchyDropIndicator(HierarchyDropPlacement placement)
		{
			if (placement == HierarchyDropPlacement::Into)
				return;

			ImVec2 itemMin = ImGui::GetItemRectMin();
			ImVec2 itemMax = ImGui::GetItemRectMax();
			float y = placement == HierarchyDropPlacement::Before ? itemMin.y : itemMax.y;
			ImGui::GetWindowDrawList()->AddLine(
				ImVec2(itemMin.x, y),
				ImVec2(itemMax.x, y),
				ImGui::GetColorU32(ImGuiCol_DragDropTarget),
				2.0f
			);
		}
	}

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

	void SceneHierarchyPanel::SetSelectedEntity(Entity entity)
	{
		m_Context->SetSelection(entity);
		m_PreviouslySelectedEntity = entity;
		m_SelectionAnchor = entity;

		// Expand hierarchy to selected entity
		if (entity != Constants::Entities::InvalidEntityID)
			m_ExpandToSelectedEntity = true;
	}

	// Selects everything drawn between the anchor and `entity`, so Shift+click grabs a contiguous run.
	void SceneHierarchyPanel::SelectRangeTo(Entity entity)
	{
		auto anchorIt = std::find(m_VisibleOrder.begin(), m_VisibleOrder.end(), m_SelectionAnchor);
		auto targetIt = std::find(m_VisibleOrder.begin(), m_VisibleOrder.end(), entity);

		if (anchorIt == m_VisibleOrder.end() || targetIt == m_VisibleOrder.end())
		{
			m_Context->SetSelection(entity);
			m_SelectionAnchor = entity;
			return;
		}

		if (anchorIt > targetIt)
			std::swap(anchorIt, targetIt);

		m_Context->SetSelection(std::vector<Entity>(anchorIt, targetIt + 1));
	}

	void SceneHierarchyPanel::RenderContextMenu()
	{
		// Only available in scene viewers
		if (m_Context->ActiveViewportViewer == nullptr || m_Context->ActiveViewportViewer->GetType() != EditorViewportViewer::Type::Scene)
			return;

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

			Entity entity;
			if (ImGui::BeginMenu("Create Controller"))
			{
				if (ImGui::MenuItem("1st Person Character"))
				{
					entity = Presets::CreateFirstPersonCharacterController(m_Context->ActiveScene());
				}
				if (ImGui::MenuItem("AI Character"))
				{
					entity = Presets::CreateAICharacterController(m_Context->ActiveScene());
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Create Primitive"))
			{
				if (ImGui::MenuItem("Cube"))
				{
					entity = Presets::CreateCube(m_Context->ActiveScene());
				}
				if (ImGui::MenuItem("Sphere"))
				{
					entity = Presets::CreateSphere(m_Context->ActiveScene());
				}
				if (ImGui::MenuItem("Quad"))
				{
					entity = Presets::CreateQuad(m_Context->ActiveScene());
				}
				if (ImGui::MenuItem("Capsule"))
				{
					entity = Presets::CreateCapsule(m_Context->ActiveScene());
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Create Light"))
			{
				if (ImGui::MenuItem("Point Light"))
				{
					entity = Presets::CreatePointLight(m_Context->ActiveScene());
				}
				if (ImGui::MenuItem("Directional Light"))
				{
					entity = Presets::CreateDirectionalLight(m_Context->ActiveScene());
				}
				if (ImGui::MenuItem("Spot Light"))
				{
					entity = Presets::CreateSpotLight(m_Context->ActiveScene());
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Create Camera"))
			{
				if (ImGui::MenuItem("3D Camera"))
				{
					entity = Presets::Create3DCamera(m_Context->ActiveScene());
				}
				if (ImGui::MenuItem("Camera From View"))
				{
					Quaternion orientation = m_Context->EditorCamera->GetOrientation();
					Vector3f position = m_Context->EditorCamera->GetPosition();
					entity = Presets::Create3DCamera(m_Context->ActiveScene(), position, orientation);
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("AI"))
			{
				if (ImGui::MenuItem("Navigation Surface"))
				{
					entity = Presets::CreateNavigationMesh(m_Context->ActiveScene());
				}
				if (ImGui::MenuItem("Navigation Grid"))
				{
					entity = Presets::CreateNavigationGrid(m_Context->ActiveScene());
				}
				if (ImGui::MenuItem("Waypoint"))
				{
					entity = Presets::CreateWaypoint(m_Context->ActiveScene());
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("UI"))
			{
				if (ImGui::MenuItem("Canvas"))
				{
					entity = Presets::CreateCanvas(m_Context->ActiveScene());
				}
				if (ImGui::MenuItem("Sprite"))
				{
					entity = Presets::CreateUISprite(m_Context->ActiveScene());
				}
				if (ImGui::MenuItem("Text"))
				{
					entity = Presets::CreateUIText(m_Context->ActiveScene());
				}
				if (ImGui::MenuItem("Button"))
				{
					entity = Presets::CreateUIButton(m_Context->ActiveScene());
				}
				if (ImGui::MenuItem("Toggle"))
				{
					entity = Presets::CreateUIToggle(m_Context->ActiveScene());
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("VFX"))
			{
				if (ImGui::MenuItem("Post Process Volume"))
				{
					entity = Presets::CreatePostProcessVolume(m_Context->ActiveScene());
				}
				ImGui::EndMenu();
			}

			if (entity != Constants::Entities::InvalidEntityID)
				CreateEntity(entity);

			ImGui::EndPopup();
		}
	}

	void SceneHierarchyPanel::RenderEntityTree()
	{
		if (!m_Context->ActiveScene())
			return;

		auto entities = m_Context->ActiveScene()->GetAllEntities();

		if (m_Context->SelectedEntity != m_PreviouslySelectedEntity)
		{
			m_PreviouslySelectedEntity = m_Context->SelectedEntity;
			m_ExpandToSelectedEntity = true;
		}

		// Rebuilt as the tree draws so Shift+click can select a run of what is actually on screen.
		m_VisibleOrder.clear();

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
		m_VisibleOrder.push_back(entity);

		ImGuiTreeNodeFlags flags = (m_Context->IsSelected(entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
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

		HandleEntityDragDrop(entity);
		HandlePrefabDragDrop(entity);

		// Select if click
		// Right-click selects immediately so the context menu opens on the correct entity, but it
		// keeps an existing multi-selection so the menu can act on all of it.
		if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && !m_Context->IsSelected(entity))
		{
			SetSelectedEntity(entity);
		}

		// Left-click selects ONLY on mouse release, AND only if the mouse wasn't dragged.
		if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
		{
			// If the user dragged the mouse (to the inspector or to another entity),
			// this threshold check prevents the selection from changing!
			if (!ImGui::IsMouseDragPastThreshold(ImGuiMouseButton_Left))
			{
				if (ImGui::GetIO().KeyShift && m_SelectionAnchor.IsValid())
				{
					SelectRangeTo(entity);
					m_ExpandToSelectedEntity = true;
				}
				else if (ImGui::GetIO().KeyCtrl)
				{
					m_Context->ToggleSelection(entity);
					m_PreviouslySelectedEntity = m_Context->SelectedEntity;
					m_SelectionAnchor = entity;
				}
				else
				{
					SetSelectedEntity(entity);
				}
			}
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
			bool isPrefabRoot = m_Context->IsEditingPrefab && entity == m_Context->PrefabRootEntity;

			if (ImGui::MenuItem("Rename Entity", "F2"))
			{
				RenameEntity(entity);
			}
			if (ImGui::MenuItem("Add Child Entity"))
			{
				CreateChildEntity(entity);
			}
			if (ImGui::MenuItem("Duplicate Entity", "CTRL+D", false, !isPrefabRoot))
			{
				DuplicateEntity(entity);
			}
			if (ImGui::MenuItem("Create Prefab From Entity", nullptr, false, !m_Context->IsEditingPrefab))
			{
				CreatePrefab(entity);
			}
			if (ImGui::MenuItem("Delete Entity", "DEL", false, !isPrefabRoot))
			{
				m_Context->PendingEntityRemovals.insert(entity);
			}
			ImGui::Separator();

			bool hasParent = entity.GetComponent<RelationshipComponent>().ParentHandle != Constants::InvalidUUID;
			if (ImGui::MenuItem("Remove Parent", nullptr, false, hasParent && !isPrefabRoot && !m_Context->IsEditingPrefab))
			{
				m_Context->ActiveScene()->MoveEntityToRootEnd(entity.GetUUID());
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
					Entity child = m_Context->ActiveScene()->GetEntity(childID);
					if (child != Constants::Entities::InvalidEntityID)
						DrawTreeNode(child);
				}
				ImGui::TreePop();
			}
		}
	}

	void SceneHierarchyPanel::HandlePrefabDragDrop(Entity entity)
	{
		// Instantiate prefab as child of the target entity
		std::string payloadType = DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::AssetPrefab);
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payloadType.c_str()))
			{
				const char* prefabPath = (const char*)payload->Data;
				auto prefab = Application::Instance().GetAssetManager().GetAssetByPath<Prefab>(prefabPath);

				Vector3f position = Vector3f(0.0f);
				Entity prefabInstance = m_Context->ActiveScene()->InstantiatePrefab(prefab, entity, &position);
				if (prefabInstance != Constants::Entities::InvalidEntityID)
				{
					SetSelectedEntity(prefabInstance);
				}
			}
			ImGui::EndDragDropTarget();
		}
	}

	void SceneHierarchyPanel::HandleEntityDragDrop(Entity entity)
	{
		std::string payloadType = DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::SceneEntity);
		if (ImGui::BeginDragDropSource())
		{
			UUID payloadUUID = entity.GetUUID();
			ImGui::SetDragDropPayload(payloadType.c_str(), &payloadUUID, sizeof(UUID));
			ImGui::Text("Move %s", entity.GetName().c_str());
			ImGui::EndDragDropSource();
		}

		const ImGuiPayload* currentPayload = ImGui::GetDragDropPayload();
		if (!currentPayload || !currentPayload->IsDataType(payloadType.c_str()))
			return;

		if (!ImGui::BeginDragDropTarget())
			return;

		UUID payloadUUID = *(const UUID*)currentPayload->Data;
		HierarchyDropPlacement placement = GetHierarchyDropPlacement();
		bool isValidPayload = placement == HierarchyDropPlacement::Into
			? CanDropEntityAsChild(payloadUUID, entity)
			: CanDropEntityAsSibling(payloadUUID, entity);

		if (isValidPayload)
		{
			DrawHierarchyDropIndicator(placement);

			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payloadType.c_str()))
			{
				payloadUUID = *(const UUID*)payload->Data;
				if (placement == HierarchyDropPlacement::Into)
				{
					m_Context->ActiveScene()->SetEntityParent(payloadUUID, entity);
				}
				else
				{
					m_Context->ActiveScene()->ReorderEntity(payloadUUID, entity.GetUUID(), placement == HierarchyDropPlacement::After);
				}
			}
		}

		ImGui::EndDragDropTarget();
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
				if (m_Context->IsEditingPrefab && m_Context->PrefabRootEntity != Constants::Entities::InvalidEntityID && payloadUUID != m_Context->PrefabRootEntity.GetUUID())
					m_Context->ActiveScene()->SetEntityParent(payloadUUID, m_Context->PrefabRootEntity);
				else if (!m_Context->IsEditingPrefab)
					m_Context->ActiveScene()->MoveEntityToRootEnd(payloadUUID);
			}
			ImGui::EndDragDropTarget();
		}
	}

	bool SceneHierarchyPanel::CanDropEntityAsChild(UUID payloadUUID, Entity targetParent)
	{
		Entity payloadEntity = m_Context->ActiveScene()->GetEntity(payloadUUID);
		if (payloadEntity == Constants::Entities::InvalidEntityID)
			return false;

		bool isSameEntity = payloadUUID == targetParent.GetUUID();
		bool isDescendant = IsAncestor(payloadEntity, targetParent);
		bool isParent = targetParent.GetUUID() == payloadEntity.GetComponent<RelationshipComponent>().ParentHandle;
		return !isSameEntity && !isDescendant && !isParent;
	}

	bool SceneHierarchyPanel::CanDropEntityAsSibling(UUID payloadUUID, Entity targetSibling)
	{
		Entity payloadEntity = m_Context->ActiveScene()->GetEntity(payloadUUID);
		if (payloadEntity == Constants::Entities::InvalidEntityID || payloadUUID == targetSibling.GetUUID())
			return false;

		UUID targetParentUUID = targetSibling.GetComponent<RelationshipComponent>().ParentHandle;
		if (targetParentUUID == Constants::InvalidUUID)
			return true;

		if (targetParentUUID == payloadUUID)
			return false;

		Entity targetParent = m_Context->ActiveScene()->GetEntity(targetParentUUID);
		if (targetParent == Constants::Entities::InvalidEntityID)
			return false;

		return !IsAncestor(payloadEntity, targetParent);
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

			current = m_Context->ActiveScene()->GetEntity(parentID);
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

			auto currentEntity = m_Context->ActiveScene()->GetEntity(currentUUID);
			auto& children = currentEntity.GetComponent<RelationshipComponent>().Children;
			for (auto& child : children)
				queue.push_back(child);
		}

		return false;
	}

	void SceneHierarchyPanel::PlaceNewEntityAtSpawnPoint(Entity entity)
	{
		if (!m_Context->SpawnPosition || entity == Constants::Entities::InvalidEntityID)
			return;

		// UI entities are laid out by their RectTransform, so a world position means nothing to them.
		if (!entity.ContainsComponent<TransformComponent>() || entity.ContainsComponent<RectTransformComponent>())
			return;

		// A parented entity's Position is relative to that parent, so only place roots.
		if (entity.ContainsComponent<RelationshipComponent>()
			&& entity.GetComponent<RelationshipComponent>().ParentHandle != Constants::InvalidUUID)
			return;

		auto& transform = entity.GetComponent<TransformComponent>();
		transform.Position = m_Context->SpawnPosition();
		transform.InvalidateWorld();
	}

	void SceneHierarchyPanel::CreateEntity(Entity entity)
	{
		PlaceNewEntityAtSpawnPoint(entity);

		SetSelectedEntity(entity);
		RenameEntity(entity);

		auto evt = UINotificationEvent(std::format("Entity {} Created", entity.GetName()));
		m_Context->EventCallback(evt);
	}

	void SceneHierarchyPanel::CreateEmptyEntity()
	{
		auto entity = m_Context->ActiveScene()->AddEntity("Empty_Entity");
		if (m_Context->IsEditingPrefab && m_Context->PrefabRootEntity != Constants::Entities::InvalidEntityID)
			m_Context->ActiveScene()->SetEntityParent(entity.GetUUID(), m_Context->PrefabRootEntity);
		CreateEntity(entity);
	}

	void SceneHierarchyPanel::CreateChildEntity(Entity parentEntity)
	{
		auto childEntity = m_Context->ActiveScene()->AddEntity("Child_Entity");

		// Set parent to new ChildEntity
		RelationshipComponent& relationship = childEntity.GetComponent<RelationshipComponent>();
		relationship.ParentHandle = parentEntity.GetUUID();
		childEntity.AttachComponent<RelationshipComponent>(relationship);

		// Set relationship on parent to include new ChildEntity
		if (!parentEntity.ContainsComponent<RelationshipComponent>())
		{
			auto& parentRelationship = parentEntity.AttachComponent<RelationshipComponent>();
			parentRelationship.Children.push_back(childEntity.GetUUID());
		}
		else
		{
			auto& parentRelationship = parentEntity.GetComponent<RelationshipComponent>();
			parentRelationship.Children.push_back(childEntity.GetUUID());
			parentEntity.AttachComponent<RelationshipComponent>(parentRelationship);
		}

		SetSelectedEntity(childEntity);
	}

	void SceneHierarchyPanel::DuplicateEntity(Entity entity)
	{
		if (entity == Constants::Entities::InvalidEntityID)
			return;
		if (m_Context->IsEditingPrefab && entity == m_Context->PrefabRootEntity)
			return;

		auto newEntity = m_Context->ActiveScene()->DuplicateEntity(entity);
		if (m_Context->IsEditingPrefab && newEntity != Constants::Entities::InvalidEntityID && newEntity.GetComponent<RelationshipComponent>().ParentHandle == Constants::InvalidUUID)
			m_Context->ActiveScene()->SetEntityParent(newEntity.GetUUID(), m_Context->PrefabRootEntity);
		SetSelectedEntity(newEntity);
	}

	// Duplicates the whole selection and leaves the copies selected, so a run of kit pieces can be
	// stamped out repeatedly.
	void SceneHierarchyPanel::DuplicateSelection()
	{
		std::vector<Entity> sources = SelectionRoots();
		if (sources.empty())
			return;

		std::vector<Entity> copies;
		copies.reserve(sources.size());

		for (Entity source : sources)
		{
			if (m_Context->IsEditingPrefab && source == m_Context->PrefabRootEntity)
				continue;

			Entity copy = m_Context->ActiveScene()->DuplicateEntity(source);
			if (copy == Constants::Entities::InvalidEntityID)
				continue;

			if (m_Context->IsEditingPrefab && copy.GetComponent<RelationshipComponent>().ParentHandle == Constants::InvalidUUID)
				m_Context->ActiveScene()->SetEntityParent(copy.GetUUID(), m_Context->PrefabRootEntity);

			copies.push_back(copy);
		}

		if (copies.empty())
			return;

		m_Context->SetSelection(copies);
		m_PreviouslySelectedEntity = m_Context->SelectedEntity;
		m_SelectionAnchor = m_Context->SelectedEntity;
		m_ExpandToSelectedEntity = true;
	}

	void SceneHierarchyPanel::RemoveSelection()
	{
		for (Entity selected : SelectionRoots())
		{
			if (m_Context->IsEditingPrefab && selected == m_Context->PrefabRootEntity)
				continue;

			m_Context->PendingEntityRemovals.insert(selected);
		}
	}

	void SceneHierarchyPanel::SelectAllEntities()
	{
		if (!m_Context->ActiveScene())
			return;

		// Only what the tree is actually showing, so a collapsed subtree is not silently included.
		m_Context->SetSelection(m_VisibleOrder);
		m_PreviouslySelectedEntity = m_Context->SelectedEntity;
		m_SelectionAnchor = m_Context->SelectedEntity;
	}

	// Selected entities whose ancestors are not themselves selected; duplicating or deleting a parent
	// already covers its subtree.
	std::vector<Entity> SceneHierarchyPanel::SelectionRoots() const
	{
		if (!m_Context->ActiveScene())
			return {};

		return m_Context->ActiveScene()->FilterToHierarchyRoots(m_Context->SelectedEntities);
	}

	void SceneHierarchyPanel::CreatePrefab(Entity entity)
	{
		std::string filePath = (ProjectManager::GetActive()->GetDefaultDirectoryForAsset(AssetType::Prefab) / (entity.GetName() + ".ebprefab")).string();
		SharedPtr<Prefab> prefab = m_Context->ActiveScene()->CreatePrefab(entity, filePath);
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
		case KeyCode::A:
			if (control)
				SelectAllEntities();
			break;
		case KeyCode::D:
			if (control)
				DuplicateSelection();
			break;
		case KeyCode::F2:
			RenameEntity(m_Context->SelectedEntity);
			break;
		case KeyCode::Delete:
			RemoveSelection();
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