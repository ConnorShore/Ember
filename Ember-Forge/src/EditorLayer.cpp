#include "EditorLayer.h"
#include "Panels/SceneHierarchyPanel.h"
#include "Panels/InspectorPanel.h"
#include "Panels/AssetManagerPanel.h"

#include <random>

namespace Ember {

	EditorLayer::EditorLayer()
		: Layer("Ember Forge"), m_EditorScene(SharedPtr<Scene>::Create("DefaultScene"))
	{
		m_Context = {
			.ActiveScene = m_EditorScene,
			.SelectedEntity = m_InvalidEntity
		};
	}

	EditorLayer::~EditorLayer()
	{
	}

	void EditorLayer::OnAttach()
	{
		// Add Panels
		m_Panels.push_back(SharedPtr<SceneHierarchyPanel>::Create(&m_Context));
		m_Panels.push_back(SharedPtr<InspectorPanel>::Create(&m_Context));
		m_Panels.push_back(SharedPtr<AssetManagerPanel>::Create(&m_Context));

		// Editor Camera Setup
		m_Camera = EditorCamera(65.0f, 1.778f, 0.1f, 500.0f);
		m_Camera.SetFocalPoint(Vector3f(0.0f, 0.0f, 0.0f));
		m_Camera.SetPitch(Math::Radians(30.0f));
		m_Camera.SetYaw(Math::Radians(45.0f));
		m_Camera.SetDistance(10.0f);

		// Output Framebuffer
		FramebufferSpecification specs;
		specs.Width = 800;
		specs.Height = 600;
		specs.AttachmentSpecs = {
			FramebufferTextureFormat::RGBA8,
			FramebufferTextureFormat::DEPTH24STENCIL8
		};
		m_OutputFramebuffer = Framebuffer::Create(specs);
	}

	void EditorLayer::OnDetach()
	{

	}

	void EditorLayer::OnEvent(Event& event)
	{
		// Handle events
		EB_CREATE_DISPATCHER(event);
		EB_DISPATCH_EVENT(KeyPressedEvent, OnKeyPressed);
		EB_DISPATCH_EVENT(MousePressedEvent, OnMouseClick);

		// Update camera
		if (m_SceneState == SceneState::Edit)
			m_Camera.OnEvent(event);

		// Propagate events to panels
		for (auto& panel : m_Panels)
			panel->OnEvent(event);
	}

	void EditorLayer::OnUpdate(TimeStep delta)
	{
		SyncEntitySelectionState();

		m_OutputFramebuffer->Bind();

		RenderAction::SetViewport(0, 0, m_OutputFramebuffer->GetSpecification().Width, m_OutputFramebuffer->GetSpecification().Height);

		switch (m_SceneState)
		{
			case SceneState::Edit:
				m_Camera.OnUpdate(delta);
				m_Context.ActiveScene->OnUpdateEdit(delta, m_Camera);
				break;
			case SceneState::Play:
				m_Context.ActiveScene->OnUpdateRuntime(delta);
				break;
			case SceneState::Pause:
			default:
				EB_CORE_ASSERT(false, "Unhandled scene state!");
		}

		m_OutputFramebuffer->Unbind();

		RenderAction::SetClearColor(Vector4f(0.0f, 0.0f, 0.0f, 1.0f));
		RenderAction::Clear(RendererAPI::RenderBit::Color);
	}

	void EditorLayer::OnImGuiRender(TimeStep delta)
	{
		ImGuizmo::BeginFrame();

		ImGui::ShowDemoWindow();

		ImGui::DockSpaceOverViewport();

		// FPS calculation (updated every 1 seconds to avoid rapid fluctuations)
		static float fps = 0.0f;
		static float fpsTimer = 0.0f;

		fpsTimer += delta.Seconds();
		if (fpsTimer >= 1.0f)
		{
			fps = 1.0f / delta.Seconds();
			fpsTimer = 0.0f;
		}

		// Menu Bar
		{
			ImGui::BeginMainMenuBar();
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6, 6));
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("New Scene", "Ctrl+N"))
				{
					NewScene();
				}

				if (ImGui::MenuItem("Open Scene", "Ctrl+O"))
				{
					OpenScene();
				}

				if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
				{
					SaveScene(false);
				}

				if (ImGui::MenuItem("Save Scene As", "Ctrl+Shift+S"))
				{
					SaveScene(true);
				}

				ImGui::EndMenu();
			}

			// Add play/pause button in center of main menu bar
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 50); // Center the button (assuming button width ~100)
			if (m_SceneState == SceneState::Play)
			{
				if (ImGui::Button("Stop"))
				{
					OnRuntimeStop();
					m_SceneState = SceneState::Edit;
				}
			}
			else
			{
				if (ImGui::Button("Play"))
				{
					OnRuntimeStart();
				}
			}

			ImGui::PopStyleVar();
			ImGui::EndMainMenuBar();
		}

		// Viewport
		{
			ImGui::Begin("Scene Viewport");
			ImGui::Text("FPS: %.1f", fps);

			// Save view port info for mouse picking and viewport resizing
			m_ViewportHovered = ImGui::IsWindowHovered();
			
			auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
			auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
			auto viewportOffset = ImGui::GetWindowPos(); // Includes tab bar height

			m_ViewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
			m_ViewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

			ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
			if (m_ViewportSize.x != viewportPanelSize.x || m_ViewportSize.y != viewportPanelSize.y)
			{
				m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };
				m_OutputFramebuffer->ViewportResize((unsigned int)m_ViewportSize.x, (unsigned int)m_ViewportSize.y);
				m_Context.ActiveScene->OnViewportResize((unsigned int)m_ViewportSize.x, (unsigned int)m_ViewportSize.y);
				m_Camera.SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
			}

			unsigned int textureID = m_OutputFramebuffer->GetColorAttachmentID(0);
			ImGui::Image((void*)textureID, ImVec2{ viewportPanelSize.x, viewportPanelSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

			// Draw Transform Gizmos for selected entity
			RenderTransformGizmos();

			ImGui::End();
		}

		// Render Panels
		for (auto& panel : m_Panels)
			panel->OnImGuiRender();

		// Delete pending entities
		RemovePendingEntities();
	}

	void EditorLayer::OnRuntimeStart()
	{
		m_Context.SelectedEntity = m_InvalidEntity;
		m_PreviousSelectedEntity = m_InvalidEntity;

		m_Context.ActiveScene = Scene::CopyScene(m_EditorScene); // Create a deep copy of the current scene for runtime
		m_Context.ActiveScene->OnViewportResize((unsigned int)m_ViewportSize.x, (unsigned int)m_ViewportSize.y);
		m_Context.ActiveScene->OnRuntimeStart();
		m_SceneState = SceneState::Play;
	}

	void EditorLayer::OnRuntimeStop()
	{
		m_Context.SelectedEntity = m_InvalidEntity;
		m_PreviousSelectedEntity = m_InvalidEntity;

		m_Context.ActiveScene = m_EditorScene; // Discard the runtime scene and revert back to the editor scene
		m_Context.ActiveScene->OnViewportResize((unsigned int)m_ViewportSize.x, (unsigned int)m_ViewportSize.y);
		m_Context.ActiveScene->OnRuntimeStop();
		m_SceneState = SceneState::Edit;
	}

	bool EditorLayer::OnKeyPressed(KeyPressedEvent& e)
	{
		// If ImGui wants to capture keyboard input for a textbox, we should not process shortcuts
		if (ImGui::GetIO().WantTextInput)
			return false;

		bool control = Input::IsKeyPressed(KeyCode::LeftControl) || Input::IsKeyPressed(KeyCode::RightControl);
		bool shift = Input::IsKeyPressed(KeyCode::LeftShift) || Input::IsKeyPressed(KeyCode::RightShift);

		KeyCode key = e.GetKeyCode();
		switch (key)
		{
			// Gizmos (Translate, Rotate, Scale) -> W, E, R (Q to disable) (T for all)
			case KeyCode::Q:
				m_GizmoType = -1;
				break;
			case KeyCode::W:
				m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
				break;
			case KeyCode::E:
				m_GizmoType = ImGuizmo::OPERATION::ROTATE;
				break;
			case KeyCode::R:
				m_GizmoType = ImGuizmo::OPERATION::SCALE;
				break;
			case KeyCode::T:
				m_GizmoType = ImGuizmo::OPERATION::UNIVERSAL;
				break;

			// Scene Hot keys
			case KeyCode::N:
				if (control)
					NewScene();
				break;
			case KeyCode::O:
				if (control)
					OpenScene();
				break;
			case KeyCode::S:
				if (control && shift)
					SaveScene(true);
				else if (control)
					SaveScene(false);
				break;

			// Entity Hot keys
			case KeyCode::Delete:
				if (m_Context.SelectedEntity != m_InvalidEntity)
					RemoveEntity(m_Context.SelectedEntity);
				break;

			case KeyCode::Space:
				if (control)
				{
					if (m_SceneState == SceneState::Play)
						m_SceneState = SceneState::Edit;
					else
						m_SceneState = SceneState::Play;
				}
				break;
		}

		return false;
	}


	bool EditorLayer::OnMouseClick(MousePressedEvent& e)
	{
		if (e.GetMouseButton() == MouseButton::Left && m_ViewportHovered)
		{
			if (m_SceneState != SceneState::Edit)
				return false;

			// If the gizmo is being hovered, we should not change selection
			if (ImGuizmo::IsOver())
				return false;

			auto [mx, my] = ImGui::GetMousePos();

			mx -= m_ViewportBounds[0].x;
			my -= m_ViewportBounds[0].y;

			Vector2f viewportSize = m_ViewportBounds[1] - m_ViewportBounds[0];
			my = viewportSize.y - my;

			int mouseX = (int)mx;
			int mouseY = (int)my;

			// Ensure we are inside the image
			if (mouseX >= 0 && mouseY >= 0 && mouseX < (int)viewportSize.x && mouseY < (int)viewportSize.y)
			{
				Entity selected = m_Context.ActiveScene->GetEntityAtPixel(mouseX, mouseY);
				m_Context.SelectedEntity = selected;
			}
		}

		return false;
	}

	void EditorLayer::SyncEntitySelectionState()
	{
		if (m_Context.SelectedEntity == m_PreviousSelectedEntity)
			return;

		// 1. Clean up the old selection safely
		if (m_PreviousSelectedEntity != Constants::Entities::InvalidEntityID && m_PreviousSelectedEntity.ContainsComponent<OutlineComponent>())
		{
			m_PreviousSelectedEntity.DetachComponent<OutlineComponent>();
			if (m_PreviousSelectedEntity.IsRootParent())
			{
				for (auto& child : m_PreviousSelectedEntity.GetAllChildren())
				{
					// DEFENSIVE CHECK: Make sure the child exists before detaching!
					if (child && child.ContainsComponent<OutlineComponent>())
						child.DetachComponent<OutlineComponent>();
				}
			}
		}

		// 2. Add outlines to the new selection safely
		if (m_Context.SelectedEntity != Constants::Entities::InvalidEntityID)
		{
			m_Context.SelectedEntity.AttachComponent(m_OutlineEntitySelectedComp);
			if (m_Context.SelectedEntity.IsRootParent())
			{
				for (auto& child : m_Context.SelectedEntity.GetAllChildren())
				{
					if (child != Constants::Entities::InvalidEntityID)
						child.AttachComponent(m_OutlineEntitySelectedComp);
				}
			}
		}

		m_PreviousSelectedEntity = m_Context.SelectedEntity;
	}

	void EditorLayer::RenderTransformGizmos()
	{
		if (m_GizmoType == -1 || m_SceneState != SceneState::Edit)
			return;

		if (m_Context.SelectedEntity == m_InvalidEntity || !m_Context.SelectedEntity.ContainsComponent<TransformComponent>())
			return;

		ImGuizmo::SetOrthographic(false);	// TODO: Support orthographic mode for 2D scenes
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(m_ViewportBounds[0].x, m_ViewportBounds[0].y, m_ViewportBounds[1].x - m_ViewportBounds[0].x, m_ViewportBounds[1].y - m_ViewportBounds[0].y);

		Matrix4f cameraProjection = m_Camera.GetProjectionMatrix();
		Matrix4f cameraView = m_Camera.GetViewMatrix();

		auto& transformComp = m_Context.SelectedEntity.GetComponent<TransformComponent>();

		// Feed ImGuizmo the WORLD transform so it draws in the correct physical location!
		Matrix4f transform = transformComp.WorldTransform;

		// Snapping Logic (Hold CTRL)
		bool snap = Input::IsKeyPressed(KeyCode::LeftControl);
		float snapValue = (m_GizmoType == ImGuizmo::OPERATION::ROTATE) ? 45.0f : 0.5f;
		float snapValues[3] = { snapValue, snapValue, snapValue };

		// Local vs World mode (Hold Shift for local)
		bool isLocal = Input::IsKeyPressed(KeyCode::LeftShift);
		ImGuizmo::MODE currentMode = isLocal ? ImGuizmo::LOCAL : ImGuizmo::WORLD;

		ImGuizmo::Manipulate(&cameraView[0][0], &cameraProjection[0][0],
			(ImGuizmo::OPERATION)m_GizmoType, (ImGuizmo::MODE)currentMode, &transform[0][0],
			nullptr, snap ? snapValues : nullptr);

		if (ImGuizmo::IsUsing())
		{
			// 'transform' is now our NEW World Matrix from the mouse drag.
			// We must convert this back into a Local Matrix before saving it!
			Matrix4f localTransform = transform;

			if (m_Context.SelectedEntity.ContainsComponent<RelationshipComponent>())
			{
				auto& relationshipComp = m_Context.SelectedEntity.GetComponent<RelationshipComponent>();
				if (relationshipComp.ParentHandle != Constants::Entities::InvalidEntityUUID)
				{
					// Fetch the parent entity
					Entity parent = m_Context.ActiveScene->GetEntity(relationshipComp.ParentHandle);
					if (parent.GetEntityHandle() != Constants::Entities::InvalidEntityID)
					{
						Matrix4f parentWorld = parent.GetComponent<TransformComponent>().WorldTransform;
						localTransform = Math::Inverse(parentWorld) * transform;
					}
				}
			}

			// Decompose the LOCAL transform back into our component variables
			Vector3f translation, rotation, scale;
			//ImGuizmo::DecomposeMatrixToComponents(&localTransform[0][0], &translation.x, &rotation.x, &scale.x);

			// TODO: Fix my decompress transform as it breaks when there is negative scaling involved (it produces NaNs in the rotation output)
			Math::DecomposeTransform(localTransform, translation, rotation, scale);

			switch (m_GizmoType)
			{
			case ImGuizmo::OPERATION::TRANSLATE:
			{
				transformComp.Position = translation;
				break;
			}
			case ImGuizmo::OPERATION::ROTATE:
			{
				if (!std::isnan(rotation.x) && !std::isnan(rotation.y) && !std::isnan(rotation.z))
					transformComp.Rotation = rotation;
				break;
			}
			case ImGuizmo::OPERATION::SCALE:
			{
				float epsilon = 0.001f;
				if (std::isnan(scale.x) || abs(scale.x) < epsilon) scale.x = epsilon;
				if (std::isnan(scale.y) || abs(scale.y) < epsilon) scale.y = epsilon;
				if (std::isnan(scale.z) || abs(scale.z) < epsilon) scale.z = epsilon;

				transformComp.Scale = scale;
				break;
			}
			case ImGuizmo::OPERATION::UNIVERSAL:
			{
				// Universal handles all three, so apply them all with protections
				transformComp.Position = translation;

				if (!std::isnan(rotation.x) && !std::isnan(rotation.y) && !std::isnan(rotation.z))
					transformComp.Rotation = rotation;

				float epsilon = 0.001f;
				if (std::isnan(scale.x) || abs(scale.x) < epsilon) scale.x = epsilon;
				if (std::isnan(scale.y) || abs(scale.y) < epsilon) scale.y = epsilon;
				if (std::isnan(scale.z) || abs(scale.z) < epsilon) scale.z = epsilon;

				transformComp.Scale = scale;
				break;
			}
			}
		}
	}

	void EditorLayer::CreateEntity()
	{
		auto entity = m_Context.ActiveScene->AddEntity("Empty_Entity");
		m_Context.SelectedEntity = entity;
	}

	void EditorLayer::RemoveEntity(Entity entity)
	{
		if (entity == Constants::Entities::InvalidEntityID)
			return;

		m_Context.PendingEntityRemovals.insert(entity);
	}

	void EditorLayer::RemovePendingEntities()
	{
		if (m_Context.PendingEntityRemovals.contains(m_Context.SelectedEntity))
			m_Context.SelectedEntity = m_InvalidEntity;

		for (auto entity : m_Context.PendingEntityRemovals)
			m_Context.ActiveScene->RemoveEntity(entity);

		m_Context.PendingEntityRemovals.clear();
	}

	void EditorLayer::NewScene()
	{
		m_EditorScene = SharedPtr<Scene>::Create("New Scene");
		m_Context.ActiveScene = m_EditorScene;

		m_Context.ActiveScene->OnViewportResize((unsigned int)m_ViewportSize.x, (unsigned int)m_ViewportSize.y);
		m_Context.SelectedEntity = {};
		m_PreviousSelectedEntity = {};

		EB_CORE_TRACE("New Scene created!");
	}

	void EditorLayer::OpenScene()
	{
		std::string sceneFile = FileDialog::OpenFile("Ember-Forge/assets/scenes", "Ember Scene (*.ebs)", "*.ebs");
		if (!sceneFile.empty())
		{
			SharedPtr<Scene> newScene = SharedPtr<Scene>::Create("Loaded Scene");
			SceneSerializer serializer(newScene);
			if (serializer.Deserialize(sceneFile))
			{
				m_EditorScene = newScene;
				m_Context.ActiveScene = m_EditorScene;
				m_Context.ActiveScene->OnViewportResize((unsigned int)m_ViewportSize.x, (unsigned int)m_ViewportSize.y);
				m_Context.ActiveScene->SetFilePath(sceneFile);

				m_Context.SelectedEntity = {};
				m_PreviousSelectedEntity = {};

				EB_CORE_TRACE("Scene loaded successfully!");
			}
			else
			{
				EB_CORE_ERROR("Failed to load scene!");
			}
		}
	}

	void EditorLayer::SaveScene(bool saveAs /* = false */)
	{
		std::string sceneName = saveAs 
			? FileDialog::SaveFile("Ember-Forge/assets/scenes/", "NewScene.ebs", "Ember Scene (*.ebs)", "*.ebs")
			: m_Context.ActiveScene->GetFilePath();

		if (!sceneName.empty())
		{
			if (m_Context.SelectedEntity != Constants::Entities::InvalidEntityID) {
				m_Context.SelectedEntity.DetachComponent<OutlineComponent>();
			}

			SceneSerializer serializer(m_Context.ActiveScene);
			serializer.Serialize(sceneName);

			if (saveAs) m_Context.ActiveScene->SetFilePath(sceneName);
			EB_CORE_TRACE("Scene saved!");
		}
	}
}
