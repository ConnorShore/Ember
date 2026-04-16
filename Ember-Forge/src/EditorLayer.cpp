#include "efpch.h"

#include "EditorLayer.h"
#include "EditorConstants.h"

#include "Panels/SceneHierarchyPanel.h"
#include "Panels/InspectorPanel.h"
#include "Panels/AssetManagerPanel.h"
#include "Panels/EnvironmentPanel.h"
#include "Panels/NotificationPanel.h"
#include "UI/DragDropTypes.h"
#include "UI/PropertyGrid.h"

#include <Ember/Render/RenderAction.h>
#include <Ember/Render/RendererAPI.h>
#include <Ember/Render/Renderer2D.h>
#include <Ember/Render/Renderer3D.h>
#include <Ember/Input/Input.h>
#include <Ember/Input/InputCode.h>
#include <Ember/Event/UIEvent.h>
#include <Ember/Core/ProjectManager.h>
#include <Ember/Utils/PlatformUtil.h>
#include <Ember/Scene/SceneSerializer.h>
#include <Ember/Asset/AssetRegistrySerializer.h>
#include <Ember/ECS/System/PhysicsSystem.h>
#include <Ember/Physics/Raycast.h>

#include <random>

namespace Ember {

	EditorLayer::EditorLayer()
		: Layer("Ember Forge")
	{
		auto defaultScene = SharedPtr<Scene>::Create("DefaultScene");
		SetNewScene(defaultScene);

		m_Context = {
			.ActiveScene = m_EditorScene,
			.EditorCamera = &m_Camera,
			.SelectedEntity = m_InvalidEntity
		};
	}

	EditorLayer::~EditorLayer()
	{
	}

	void EditorLayer::OnAttach()
	{
		// Setup theme
		SetupImGuiTheme();

		// Add Panels
		m_Panels.push_back(SharedPtr<SceneHierarchyPanel>::Create(&m_Context));
		m_Panels.push_back(SharedPtr<AssetManagerPanel>::Create(&m_Context));
		m_Panels.push_back(SharedPtr<EnvironmentPanel>::Create(&m_Context));
		m_Panels.push_back(SharedPtr<InspectorPanel>::Create(&m_Context));
		m_Panels.push_back(SharedPtr<NotificationPanel>::Create(&m_Context));

		// Default Assets
		LoadDefaultAssets();

		// Editor Camera Setup
		m_Camera = EditorCamera(65.0f, 1.778f, 0.1f, 5000.0f);
		m_Camera.SetFocalPoint(Vector3f(0.0f, 0.0f, 0.0f));
		m_Camera.SetPitch(Math::Radians(30.0f));
		m_Camera.SetYaw(Math::Radians(45.0f));
		m_Camera.SetDistance(6.0f);

		// Output Framebuffer
		FramebufferSpecification specs;
		specs.Width = 800;
		specs.Height = 600;
		specs.AttachmentSpecs = {
			FramebufferTextureFormat::RGBA8,
			FramebufferTextureFormat::Depth24Stencil8
		};
		m_OutputFramebuffer = Framebuffer::Create(specs);

		for (auto& panel : m_Panels)
			panel->OnAttach();

		// Load play / pause textures
		m_ToolbarProps.PlayButtonTextureID = Application::Instance().GetAssetManager().Load<Texture2D>("Ember-Forge/assets/icons/Play.png")->GetID();
		m_ToolbarProps.StopButtonTextureID = Application::Instance().GetAssetManager().Load<Texture2D>("Ember-Forge/assets/icons/Stop.png")->GetID();
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
		if (m_Context.CurrentSceneState == SceneState::Edit)
			m_Camera.OnEvent(event);

		// Propagate events to panels
		for (auto& panel : m_Panels)
			panel->OnEvent(event);
	}

	void EditorLayer::OnUpdate(TimeStep delta)
	{
		SyncEntitySelectionState();

		for (auto& panel : m_Panels)
			panel->OnUpdate(delta);

		m_OutputFramebuffer->Bind();

		RenderAction::SetViewport(0, 0, m_OutputFramebuffer->GetSpecification().Width, m_OutputFramebuffer->GetSpecification().Height);

		switch (m_Context.CurrentSceneState)
		{
			case SceneState::Edit:
			{
				m_Camera.OnUpdate(delta);
				m_Context.ActiveScene->OnUpdateEdit(delta, m_Camera);
				break;
			}
			case SceneState::Play:
			{
				m_Context.ActiveScene->OnUpdateRuntime(delta);
				break;
			}
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
		ImGui::DockSpaceOverViewport();

		//ImGui::ShowDemoWindow();

		// Menu Bar
		RenderMenuBar();
		DrawToolbar();
		RenderSceneViewport();

		// Project Settings Pop up
		if (m_ShowProjectSettingsPopup)
		{
			ImGui::OpenPopup(m_ProjectSettingsDialog.GetPopupName().c_str());
			m_ShowProjectSettingsPopup = false;
		}
		m_ProjectSettingsDialog.OnImGuiRender();

		// Render stats overlay
		if (m_ShowStatsWindow)
			RenderStatsOverlay(delta);

		// Render Panels
		for (auto& panel : m_Panels)
			panel->OnImGuiRender();

		// Pop up for new project
		RenderNewProjectPopup();

		// Deferred removal - entities/components are queued during iteration and removed at frame end
		RemovePendingComponents();
		RemovePendingEntities();
	}

	void EditorLayer::LoadDefaultAssets()
	{
		auto& assetManager = Application::Instance().GetAssetManager();

		// Textures
		auto pointLightTex = assetManager.Load<Texture2D>(EditorConstants::Assets::PointLightTexUUID, EditorConstants::Assets::PointLightTex, "Ember-Forge/assets/icons/PointLight.png");
		auto directionalLightTex = assetManager.Load<Texture2D>(EditorConstants::Assets::DirectionalLightTexUUID, EditorConstants::Assets::DirectionalLightTex, "Ember-Forge/assets/icons/DirectionalLight.png");
		auto spotLightTex = assetManager.Load<Texture2D>(EditorConstants::Assets::SpotLightTexUUID, EditorConstants::Assets::SpotLightTex, "Ember-Forge/assets/icons/SpotLight.png");
		auto cameraTex = assetManager.Load<Texture2D>(EditorConstants::Assets::CameraTexUUID, EditorConstants::Assets::CameraTex, "Ember-Forge/assets/icons/Camera.png");
	}

	void EditorLayer::OnRuntimeStart()
	{
		m_Context.ActiveScene = Scene::CopyScene(m_EditorScene); // Create a deep copy of the current scene for runtime
		m_Context.ActiveScene->OnViewportResize(static_cast<uint32_t>(m_ViewportSize.x), static_cast<uint32_t>(m_ViewportSize.y));
		m_Context.ActiveScene->OnRuntimeStart();
		m_Context.CurrentSceneState = SceneState::Play;
	}

	void EditorLayer::OnRuntimeStop()
	{
		m_Context.ActiveScene = m_EditorScene; // Discard the runtime scene and revert back to the editor scene
		m_Context.ActiveScene->OnViewportResize(static_cast<uint32_t>(m_ViewportSize.x), static_cast<uint32_t>(m_ViewportSize.y));
		m_Context.ActiveScene->OnRuntimeStop();
		m_Context.CurrentSceneState = SceneState::Edit;
	}

	void EditorLayer::RenderMenuBar()
	{
		ImGui::BeginMainMenuBar();
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6, 6));
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New Project", "Ctrl+Shift+N"))
			{
				NewProject();
			}
			if (ImGui::MenuItem("Open Project", "Ctrl+Shift+O"))
			{
				OpenProject();
			}

			ImGui::Separator();
			bool projectExists = ProjectManager::GetActive() != nullptr;

			if (ImGui::MenuItem("New Scene", "Ctrl+N", false, projectExists))
			{
				NewScene();
			}

			if (ImGui::MenuItem("Open Scene", "Ctrl+O", false, projectExists))
			{
				OpenScene();
			}

			if (ImGui::MenuItem("Save Scene", "Ctrl+S", false, projectExists))
			{
				SaveScene(false);
			}

			if (ImGui::MenuItem("Save Scene As", "Ctrl+Shift+S", false, projectExists))
			{
				SaveScene(true);
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Project"))
		{
			if (ImGui::MenuItem("Project Settings"))
			{
				m_ShowProjectSettingsPopup = true;
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Editor"))
		{
			if (ImGui::BeginMenu("Tool Windows"))
			{
				ImGui::MenuItem("Render Stats", nullptr, &m_ShowStatsWindow);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Debug"))
			{
				auto physicsSystem = Application::Instance().GetSystemManager().GetSystem<PhysicsSystem>();
				if (physicsSystem)
				{
					auto& debugSettings = physicsSystem->GetDebugRenderSettings();

					ImGui::MenuItem("Show Physics Colliders", nullptr, &debugSettings.Enabled);
					if (debugSettings.Enabled)
					{
						ImGui::Separator();

						ImGui::MenuItem("Draw Shapes", nullptr, &debugSettings.DrawColliders);
						ImGui::MenuItem("Draw Contact Points", nullptr, &debugSettings.DrawContactPoints);
						ImGui::MenuItem("Draw AABBs", nullptr, &debugSettings.DrawColliderAxes);
					}
				}
				ImGui::EndMenu();
			}

			ImGui::EndMenu();
		}

		ImGui::PopStyleVar();
		ImGui::EndMainMenuBar();
	}

	void EditorLayer::RenderSceneViewport()
	{
		ImGui::Begin("Scene");

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
			m_OutputFramebuffer->ViewportResize(static_cast<uint32_t>(m_ViewportSize.x), static_cast<uint32_t>(m_ViewportSize.y));
			m_Context.ActiveScene->OnViewportResize(static_cast<uint32_t>(m_ViewportSize.x), static_cast<uint32_t>(m_ViewportSize.y));
			m_Camera.SetViewportSize(static_cast<uint32_t>(m_ViewportSize.x), static_cast<uint32_t>(m_ViewportSize.y));
		}

		uint32_t textureID = m_OutputFramebuffer->GetColorAttachmentID(0);
		ImGui::Image(reinterpret_cast<void*>(static_cast<uintptr_t>(textureID)), ImVec2{ viewportPanelSize.x, viewportPanelSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

		// Drag drop zone for models and prefabs
		if (ImGui::BeginDragDropTarget())
		{
			// Models
			{
				std::string payloadType = DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::AssetModel);
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payloadType.c_str()))
				{
					std::string filePath = std::string((char*)payload->Data, payload->DataSize);
					CreateEntityFromModel(filePath);
				}
			}

			// Prefabs
			{
				std::string payloadType = DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::AssetPrefab);
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payloadType.c_str()))
				{
					std::string filePath = std::string((char*)payload->Data, payload->DataSize);
					CreateEntityFromPrefab(filePath);
				}
			}

			ImGui::EndDragDropTarget();
		}

		// Draw Transform Gizmos for selected entity
		RenderTransformGizmos();

		ImGui::End();
	}

	void EditorLayer::RenderNewProjectPopup()
	{
		if (m_NewProjectSettings.ShowProjectSettingsPopup)
		{
			ImGui::OpenPopup("New Project");
			m_NewProjectSettings.ShowProjectSettingsPopup = false;
		}

		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(600, 200), ImGuiCond_Appearing);

		if (ImGui::BeginPopupModal("New Project", NULL, ImGuiWindowFlags_NoSavedSettings))
		{
			if (UI::PropertyGrid::Begin("NewProjectTable"))
			{
				UI::PropertyGrid::InputText("Project Name", m_NewProjectSettings.ProjectName);

				auto browseFunc = [&]() {
					std::string selectedDir = FileDialog::OpenDirectory();
					if (!selectedDir.empty())
						m_NewProjectSettings.ProjectDirectory = selectedDir;
					};

				UI::PropertyGrid::DirectoryInput("Location", m_NewProjectSettings.ProjectDirectory, browseFunc);

				UI::PropertyGrid::End();
			}

			ImGui::Separator();
			ImGui::Dummy(ImVec2(0.0f, ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing()));

			// Actions
			bool isValid = !m_NewProjectSettings.ProjectName.empty() && !m_NewProjectSettings.ProjectDirectory.empty();

			if (!isValid) ImGui::BeginDisabled();

			if (ImGui::Button("Create", ImVec2(120, 0)))
			{
				std::filesystem::path projectDirPath = std::filesystem::path(m_NewProjectSettings.ProjectDirectory) / m_NewProjectSettings.ProjectName;
				std::filesystem::create_directories(projectDirPath);

				std::string projectNameFinal = m_NewProjectSettings.ProjectName + ".ebproj";
				std::filesystem::path fullProjectPath = projectDirPath / projectNameFinal;

				auto project = ProjectManager::NewProject(fullProjectPath.string());

				OpenScene(project->GetStartScenePath().string());

				if (auto assetPanel = GetPanel<AssetManagerPanel>())
					assetPanel->UpdateAssetDirectory(project->GetAssetDirectory());

				Application::Instance().GetAssetManager().ClearAssets();

				ImGui::CloseCurrentPopup();
			}

			if (!isValid) ImGui::EndDisabled();

			ImGui::SameLine();

			if (ImGui::Button("Cancel", ImVec2(120, 0)))
			{
				m_NewProjectSettings.ProjectName = "";
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	bool EditorLayer::OnKeyPressed(KeyPressedEvent& e)
	{
		// If ImGui wants to capture keyboard input for a textbox, we should not process shortcuts
		if (ImGui::GetIO().WantTextInput)
			return false;

		bool activeProject = ProjectManager::GetActive() != nullptr;
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
				if (shift && control)
					NewProject();
				else if (activeProject && control)
					NewScene();
				break;
			case KeyCode::O:
				if (shift && control)
					OpenProject();
				else if (activeProject && control)
					OpenScene();
				break;
			case KeyCode::S:
				if (activeProject && control && shift)
					SaveScene(true);
				else if (activeProject && control)
					SaveScene(false);
				break;

			// Entity Hot keys
			case KeyCode::Delete:
				if (m_Context.SelectedEntity != m_InvalidEntity)
					RemoveEntity(m_Context.SelectedEntity);
				break;

			case KeyCode::Enter:
				if (control)
				{
					if (m_Context.CurrentSceneState == SceneState::Play)
						OnRuntimeStop();
					else
						OnRuntimeStart();
				}
				break;
		}

		return false;
	}


	bool EditorLayer::OnMouseClick(MousePressedEvent& e)
	{
		if (e.GetMouseButton() == MouseButton::Left && m_ViewportHovered)
		{
			if (m_Context.CurrentSceneState != SceneState::Edit)
				return false;

			// If a gizmo is drawn and the mouse is over it, we should not change the selected entity
			bool isGizmoDrawn = m_Context.SelectedEntity != m_InvalidEntity && m_GizmoType != -1;
			if (isGizmoDrawn && ImGuizmo::IsOver())
				return false;

			auto [mx, my] = ImGui::GetMousePos();

			// Convert screen-space mouse coords to viewport-local with Y flipped for OpenGL
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

		// Clean up the old selection safely
		if (m_PreviousSelectedEntity != Constants::Entities::InvalidEntityID && m_PreviousSelectedEntity.ContainsComponent<OutlineComponent>())
		{
			RemoveComponentFromEntity<OutlineComponent>(m_PreviousSelectedEntity);
			if (m_PreviousSelectedEntity.IsRootParent())
			{
				for (auto& child : m_PreviousSelectedEntity.GetAllChildren())
				{
					if (child && child.ContainsComponent<OutlineComponent>())
						RemoveComponentFromEntity<OutlineComponent>(child);
				}
			}
		}

		// Add outlines to the new selection safely
		if (m_Context.SelectedEntity != Constants::Entities::InvalidEntityID)
		{
			OutlineEntity(m_Context.SelectedEntity);
			if (m_Context.SelectedEntity.IsRootParent())
			{
				for (auto& child : m_Context.SelectedEntity.GetAllChildren())
				{
					if (child != Constants::Entities::InvalidEntityID)
					{
						OutlineEntity(child);
					}
				}
			}
		}

 		m_PreviousSelectedEntity = m_Context.SelectedEntity;
	}

	void EditorLayer::RenderTransformGizmos()
	{
		if (m_GizmoType == -1 || m_Context.CurrentSceneState != SceneState::Edit)
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
			// ImGuizmo returns a new world-space matrix; convert back to local space
			// by multiplying with the inverse of the parent's world transform
			Matrix4f localTransform = transform;

			if (m_Context.SelectedEntity.ContainsComponent<RelationshipComponent>())
			{
				auto& relationshipComp = m_Context.SelectedEntity.GetComponent<RelationshipComponent>();
				if (relationshipComp.ParentHandle != Constants::InvalidUUID)
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

	void EditorLayer::DrawToolbar()
	{
		// Prevent the user from resizing the dock node itself
		ImGuiWindowClass window_class;
		window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_NoResize;
		ImGui::SetNextWindowClass(&window_class);


		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 4));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

		ImGui::Begin("Toolbar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

		float windowWidth = ImGui::GetWindowContentRegionMax().x;
		float iconSize = 24.0f;


		// Center the button and make it transparent
		float buttonSizeWithPadding = iconSize + (ImGui::GetStyle().FramePadding.x * 2.0f);
		ImGui::SetCursorPosX((windowWidth * 0.5f) - (buttonSizeWithPadding * 0.5f));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

		// (Optional) If you want to customize the hover color so it looks sleeker:
		 ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
		 ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.2f, 0.5f));

		if (m_Context.CurrentSceneState == SceneState::Play)
		{
			if (ImGui::ImageButton("StopButton", m_ToolbarProps.StopButtonTextureID, ImVec2(iconSize, iconSize)))
			{
				OnRuntimeStop();
				m_Context.CurrentSceneState = SceneState::Edit;
			}
		}
		else
		{
			if (ImGui::ImageButton("PlayButton", m_ToolbarProps.PlayButtonTextureID, ImVec2(iconSize, iconSize)))
			{
				OnRuntimeStart();
			}
		}

		// Pop the transparent button color!
		ImGui::PopStyleColor(3);

		ImGui::End(); 
		ImGui::PopStyleVar(2);
	}

	void EditorLayer::RenderStatsOverlay(TimeStep delta)
	{
		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking |
			ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
			ImGuiWindowFlags_NoMove;

		ImVec2 windowPos = ImVec2(m_ViewportBounds[1].x - 10.0f, m_ViewportBounds[0].y + 10.0f);
		ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, ImVec2(1.0f, 0.0f));	// Pin to top-right corner of the viewport with right alignment

		ImGui::SetNextWindowBgAlpha(0.35f);

		if (ImGui::Begin("Stats Overlay", nullptr, windowFlags))
		{
			ImGui::Text("Renderer Stats");
			ImGui::Separator();
			ImGui::Text("FPS: %.1f", CalculateFPS(delta));
			ImGui::Text("Entities: %d", m_Context.ActiveScene->GetAllEntities().size());
		}
		ImGui::End();
	}

	// Averages frame count over 1-second intervals for a stable FPS readout
	float EditorLayer::CalculateFPS(TimeStep delta)
	{
		static float fpsTimer = 0.0f;
		static float fps = 0.0f;
		static int frameCount = 0;    

		fpsTimer += delta.Seconds();
		frameCount++;

		if (fpsTimer >= 1.0f)
		{
			fps = (float)frameCount / fpsTimer;

			fpsTimer = 0.0f;
			frameCount = 0;
		}

		return fps;
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

		for (auto entity : m_Context.PendingEntityRemovals) {
			std::string entityName = entity.GetName();
			m_Context.ActiveScene->RemoveEntity(entity);

			auto evt = UINotificationEvent(std::format("Entity {} Removed", entityName));
			m_Context.EventCallback(evt);
		}

		m_Context.PendingEntityRemovals.clear();
	}

	void EditorLayer::RemovePendingComponents()
	{
		for (auto& [entity, components] : m_Context.PendingComponentRemovals)
		{
			for (ComponentType componentType : components)
			{
				if (entity.ContainsComponent(componentType))
				{
					entity.DetachComponent(componentType);
				}
			}
		}

		m_Context.PendingComponentRemovals.clear();
	}

	void EditorLayer::CreateEntityFromModel(const std::string& modelFilePath)
	{
		Entity modelEntity = m_Context.ActiveScene->InstantiateModel(modelFilePath);
		m_Context.SelectedEntity = modelEntity;
	}

	void EditorLayer::CreateEntityFromPrefab(const std::string& prefabFilePath)
	{
		auto& assetManager = Application::Instance().GetAssetManager();
		auto prefabAsset = assetManager.Load<Prefab>(prefabFilePath);

		// Instantiate prefab at origin
		// TODO: Maybe we should spawn it at the mouse position in the viewport instead of always at the origin
		// using raycasting
		Vector3f origin = Vector3f(0.0f);
		Entity prefEntity = m_Context.ActiveScene->InstantiatePrefab(prefabAsset, &origin);
		m_Context.SelectedEntity = prefEntity;
	}

	void EditorLayer::OutlineEntity(Entity entity)
	{
		// If the outline was queued for removal (e.g. from deselection), cancel it instead of double-adding
		bool removed = CancelComponentRemoval<OutlineComponent>(entity);
		if (!removed)
			entity.AttachComponent(m_OutlineEntitySelectedComp);
	}

	void EditorLayer::NewProject()
	{
		if (m_NewProjectSettings.ProjectDirectory.empty())
		{
			m_NewProjectSettings.ProjectDirectory = std::filesystem::current_path().string();
		}

		m_NewProjectSettings.ProjectName = "NewProject";
		m_NewProjectSettings.ShowProjectSettingsPopup = true;
	}

	void EditorLayer::OpenProject()
	{
		std::string projectFile = FileDialog::OpenFile("", "Ember Project (*.ebproj)", "*.ebproj");
		if (projectFile.empty())
			return;

		auto project = ProjectManager::LoadProject(projectFile);

		// Load assets for project
		auto assetPanel = GetPanel<AssetManagerPanel>();
		if (assetPanel != nullptr)
			assetPanel->UpdateAssetDirectory(project->GetAssetDirectory());

		// Clear and reload default engine assets before loading the scene so all
		// asset UUIDs referenced by the scene (e.g. skybox texture) are resolvable.
		auto& assetManager = Application::Instance().GetAssetManager();
		assetManager.ClearAssets();
		assetManager.LoadDefaults();

		// Deserialize project assets
		std::string assetFilePath = (project->GetAssetDirectory() / "Assets.eba").string();
		AssetRegistrySerializer assetSerializer(&assetManager);
		assetSerializer.Deserialize(assetFilePath);

		// Load the default scene for the project (assets must be ready first)
		OpenScene(project->GetStartScenePath().string());
	}

	void EditorLayer::NewScene()
	{
		SharedPtr<Scene> newScene = SharedPtr<Scene>::Create("New Scene");
		SetNewScene(newScene);

		auto evt = UINotificationEvent("New Scene created!");
		m_Context.EventCallback(evt);
	}

	void EditorLayer::OpenScene(const std::string& scenePath /* = "" */)
	{
		std::string sceneFile = scenePath;
		if (sceneFile.empty())
		{
			const char* sceneDirectory = ProjectManager::GetActive()->GetAssetDirectory().string().c_str();
			sceneFile = FileDialog::OpenFile(sceneDirectory, "Ember Scene (*.ebs)", "*.ebs");
		}

		if (!sceneFile.empty())
		{
			SharedPtr<Scene> newScene = SharedPtr<Scene>::Create("Loaded Scene");
			newScene->SetFilePath(sceneFile);

			SceneSerializer serializer(newScene);
			if (serializer.Deserialize(sceneFile))
			{
				SetNewScene(newScene);

				auto evt = UINotificationEvent(std::format("Scene opened: {}", std::filesystem::path(sceneFile).filename().string()));
				m_Context.EventCallback(evt);
			}
			else
			{
				auto evt = UINotificationEvent("Failed to load scene!", UINotificationEvent::Severity::Error);
				m_Context.EventCallback(evt);
			}
		}
	}

	void EditorLayer::SaveScene(bool saveAs /* = false */)
	{
		std::string sceneDirectory = ProjectManager::GetActive()->GetAssetDirectory().string();
		std::string sceneName = saveAs
			? FileDialog::SaveFile(sceneDirectory.c_str(), "NewScene.ebs", "Ember Scene (*.ebs)", "*.ebs")
			: m_Context.ActiveScene->GetFilePath();

		if (!sceneName.empty())
		{
			// Strip editor-only outline components before serializing
			if (m_Context.SelectedEntity != Constants::Entities::InvalidEntityID)
				m_Context.SelectedEntity.DetachComponent<OutlineComponent>();

			// Serialize scene
			SceneSerializer sceneSerializer(m_Context.ActiveScene);
			sceneSerializer.Serialize(sceneName);

			// Re-apply outline component after saving so the user doesn't lose their selection highlight
			if (m_Context.SelectedEntity != Constants::Entities::InvalidEntityID)
				OutlineEntity(m_Context.SelectedEntity);

			// Serialize materials (in case their values changed)
			auto materials = Application::Instance().GetAssetManager().GetAssetsOfType<MaterialInstance>();
			for (auto& material : materials)
			{
				if (!material->IsEngineAsset() && !material->GetFilePath().empty())
				{
					MaterialSerializer::Serialize(material->GetFilePath(), material);
				}
			}

			// Serialize physics materials as well
			auto physicsMaterials = Application::Instance().GetAssetManager().GetAssetsOfType<PhysicsMaterial>();
			for (auto& physMat : physicsMaterials)
			{
				if (!physMat->IsEngineAsset() && !physMat->GetFilePath().empty())
				{
					PhysicsMaterialSerializer::Serialize(physMat->GetFilePath(), physMat);
				}
			}

			// Serialize assets
			std::filesystem::path assetFilePath = ProjectManager::GetActive()->GetAssetDirectory() / "Assets.eba";
			AssetRegistrySerializer assetSerializer(&Application::Instance().GetAssetManager());
			assetSerializer.Serialize(assetFilePath.string());

			if (saveAs) m_Context.ActiveScene->SetFilePath(sceneName);

			// Save project as well to update any project settings
			ProjectManager::SaveActiveProject();

			auto evt = UINotificationEvent(std::format("Scene saved: {}", std::filesystem::path(sceneName).filename().string()));
			m_Context.EventCallback(evt);
		}
	}

	void EditorLayer::SetNewScene(SharedPtr<Scene> newScene)
	{
		if (m_Context.ActiveScene != nullptr)
			m_Context.ActiveScene->OnDetach();

		m_EditorScene = newScene;
		m_Context.ActiveScene = newScene;

		m_Context.ActiveScene->OnAttach();

		m_Context.ActiveScene->OnViewportResize(static_cast<uint32_t>(m_ViewportSize.x), static_cast<uint32_t>(m_ViewportSize.y));
		m_Context.SelectedEntity = {};
		m_PreviousSelectedEntity = {};
	}

	void EditorLayer::SetupImGuiTheme()
	{
		ImGuiIO& io = ImGui::GetIO();
		io.Fonts->AddFontFromFileTTF("Ember-Forge/assets/fonts/Roboto-Regular.ttf", 16.0f);

		ImGuiStyle& style = ImGui::GetStyle();
		ImVec4* colors = style.Colors;

		// --- Sizing & Spacing (Spacious & Modern) ---
		style.WindowPadding = ImVec2(12.0f, 12.0f);
		style.FramePadding = ImVec2(8.0f, 6.0f);
		style.ItemSpacing = ImVec2(10.0f, 8.0f);
		style.ItemInnerSpacing = ImVec2(6.0f, 6.0f);
		style.IndentSpacing = 20.0f;
		style.ScrollbarSize = 14.0f;
		style.GrabMinSize = 12.0f;

		// --- Borders & Rounding (Flat, subtle rounding) ---
		style.WindowRounding = 4.0f;
		style.ChildRounding = 4.0f;
		style.FrameRounding = 3.0f;
		style.PopupRounding = 4.0f;
		style.ScrollbarRounding = 3.0f;
		style.GrabRounding = 3.0f;
		style.TabRounding = 3.0f;

		style.WindowBorderSize = 1.0f;
		style.FrameBorderSize = 0.0f;
		style.PopupBorderSize = 1.0f;

		// --- Base Colors (Deep Greys) ---
		ImVec4 textBase = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
		ImVec4 textMuted = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);

		ImVec4 bgDarkest = ImVec4(0.08f, 0.08f, 0.08f, 1.00f); // Main window backgrounds
		ImVec4 bgMid = ImVec4(0.12f, 0.12f, 0.12f, 1.00f); // Child windows
		ImVec4 bgLight = ImVec4(0.16f, 0.16f, 0.16f, 1.00f); // Input fields, empty frames

		ImVec4 borderCol = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);

		// --- Accent Colors (Industrial Orange) ---
		ImVec4 accentBase = ImVec4(0.88f, 0.40f, 0.10f, 1.00f); // #E2681B
		ImVec4 accentHovered = ImVec4(0.95f, 0.47f, 0.15f, 1.00f); // Brighter on hover
		ImVec4 accentActive = ImVec4(0.80f, 0.35f, 0.08f, 1.00f); // Darker on click

		// --- Apply Colors ---
		colors[ImGuiCol_Text] = textBase;
		colors[ImGuiCol_TextDisabled] = textMuted;

		// Backgrounds
		colors[ImGuiCol_WindowBg] = bgDarkest;
		colors[ImGuiCol_ChildBg] = bgMid;
		colors[ImGuiCol_PopupBg] = bgDarkest;
		colors[ImGuiCol_Border] = borderCol;
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

		// Frames (Inputs, Checkboxes)
		colors[ImGuiCol_FrameBg] = bgLight;
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);

		// Titles
		colors[ImGuiCol_TitleBg] = bgDarkest;
		colors[ImGuiCol_TitleBgActive] = bgDarkest;
		colors[ImGuiCol_TitleBgCollapsed] = bgDarkest;

		// Buttons (Keep grey base, tint orange on hover for a cleaner look)
		colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_ButtonHovered] = accentHovered;
		colors[ImGuiCol_ButtonActive] = accentActive;

		// Headers (TreeNodes, Selectables)
		colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_HeaderHovered] = accentHovered;
		colors[ImGuiCol_HeaderActive] = accentActive;

		// Tabs
		colors[ImGuiCol_Tab] = bgMid;
		colors[ImGuiCol_TabHovered] = accentHovered;
		colors[ImGuiCol_TabActive] = accentBase;
		colors[ImGuiCol_TabUnfocused] = bgDarkest;
		colors[ImGuiCol_TabUnfocusedActive] = bgMid;

		// Sliders & Grabs
		colors[ImGuiCol_SliderGrab] = accentBase;
		colors[ImGuiCol_SliderGrabActive] = accentActive;

		// Drag Drop
		colors[ImGuiCol_DragDropTarget] = accentHovered;

		// Separators
		colors[ImGuiCol_Separator] = borderCol;
		colors[ImGuiCol_SeparatorHovered] = accentBase;
		colors[ImGuiCol_SeparatorActive] = accentActive;

		// Resize Grips
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_ResizeGripHovered] = accentHovered;
		colors[ImGuiCol_ResizeGripActive] = accentActive;
	}
}
