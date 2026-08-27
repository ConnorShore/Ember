#include "efpch.h"

#include "EditorLayer.h"
#include "EditorConstants.h"

#include "Panels/SceneHierarchyPanel.h"
#include "Panels/AssetManagerPanel.h"
#include "Panels/EnvironmentPanel.h"
#include "Panels/NotificationPanel.h"
#include "Panels/LogPanel.h"
#include "Panels/AnimationScrubberPanel.h"
#include "Panels/Inspector/InspectorPanel.h"

#include "UI/DragDropTypes.h"
#include "UI/PropertyGrid.h"
#include "Utils/ActiveNavMeshRenderer.h"

#include <Ember/ECS/System/UIInputSystem.h>
#include <Ember/Render/RenderAction.h>
#include <Ember/Render/RendererAPI.h>
#include <Ember/Render/Renderer2D.h>
#include <Ember/Render/Renderer3D.h>
#include <Ember/Input/Input.h>
#include <Ember/Input/InputCode.h>
#include <Ember/Event/UIEvent.h>
#include <Ember/Core/Paths.h>
#include <Ember/Core/ProjectManager.h>
#include <Ember/Utils/PlatformUtil.h>
#include <Ember/Scene/SceneSerializer.h>
#include <Ember/Scene/SceneManager.h>
#include <Ember/Animation/Animation.h>
#include <Ember/Animation/AnimationSerializer.h>
#include <Ember/Asset/Serializers/AssetRegistrySerializer.h>
#include <Ember/ECS/System/PhysicsSystem.h>
#include <Ember/ECS/System/AISystem.h>
#include <Ember/ECS/System/AnimationSystem.h>
#include <Ember/ECS/System/VisibilitySystem.h>
#include <Ember/Physics/Raycast.h>
#include <Ember/Animation/AnimationController.h>

#include <random>
#include <thread>
#include <exception>
#include <fstream>

namespace Ember {
	
	static Entity ResolveEntityInScene(SharedPtr<Scene> scene, Entity entity)
	{
		if (!scene)
			return Entity();

		return scene->GetEntityByHandle(entity.GetEntityHandle());
	}

	EditorLayer::EditorLayer()
		: Layer("Ember Forge")
	{
		m_Context = {
			.EditorCamera = &m_Camera,
			.SelectedEntity = m_InvalidEntity
		};
		m_Context.Preferences = &m_Preferences;
		m_Context.SpawnPosition = [this]() { return GetSpawnPosition(); };

		m_EditorRenderPassSettings = {
			.ActiveCamera = &m_Camera,
			.CameraTransform = Math::Inverse(m_Camera.GetViewMatrix()),
			.RenderMask = FilterPreset::All,
			.VolumeMask = FilterPreset::All
		};
		m_EditorRenderPassSettings.PreDebugDrawCallback = ViewportGizmoController::DrawSceneDebugGizmos;

		// Provide a blank scene so the editor has a valid active scene before any project is loaded.
		// It will be replaced cleanly when a project is opened via the deferred scene swap.
		auto defaultScene = SharedPtr<Scene>::Create("DefaultScene", "");
		SetNewScene(defaultScene);
	}

	EditorLayer::~EditorLayer()
	{
	}

	void EditorLayer::OnAttach()
	{
		// Missing or unreadable preferences just leave the struct on its defaults.
		m_Preferences.Load();

		// Setup theme
		SetupImGuiTheme();

		// Add Panels
		m_Panels.push_back(SharedPtr<SceneHierarchyPanel>::Create(&m_Context));
		m_Panels.push_back(SharedPtr<AnimationScrubberPanel>::Create(&m_Context));
		m_Panels.push_back(SharedPtr<AssetManagerPanel>::Create(&m_Context));
		m_Panels.push_back(SharedPtr<EnvironmentPanel>::Create(&m_Context));
		m_Panels.push_back(SharedPtr<InspectorPanel>::Create(&m_Context));
		m_Panels.push_back(SharedPtr<NotificationPanel>::Create(&m_Context));
		m_Panels.push_back(SharedPtr<LogPanel>::Create(&m_Context));

		// Default Assets
		LoadDefaultAssets();

		// Editor Camera Setup
		m_Camera = EditorCamera(65.0f, 1.778f, 0.3f, 300.0f);
		m_Camera.SetFocalPoint(Vector3f(0.0f, 0.0f, 0.0f));
		m_Camera.SetPitch(Math::Radians(30.0f));
		m_Camera.SetYaw(Math::Radians(45.0f));
		m_Camera.SetDistance(6.0f);

		// Output Framebuffer (and preview FBO mirrors its size so the scene's internal
		// render pass FBOs don't have to be reallocated when rendering the camera preview)
		{
			FramebufferSpecification specs;
			specs.Width = 800;
			specs.Height = 600;
			specs.AttachmentSpecs = {
				FramebufferTextureFormat::RGBA8,
				FramebufferTextureFormat::Depth24Stencil8
			};
			m_OutputFramebuffer = Framebuffer::Create(specs);
			m_CameraPreviewFramebuffer = Framebuffer::Create(specs);

			// Notify scene of the initial viewport size so render pass FBOs are sized correctly from the start
			m_ViewportSize = { (float)specs.Width, (float)specs.Height };
			if (auto activeScene = m_Context.ActiveScene())
				activeScene->OnViewportResize(specs.Width, specs.Height);
			m_Camera.SetViewportSize(specs.Width, specs.Height);
		}

		for (auto& panel : m_Panels)
			panel->OnAttach();

		// Load play / pause textures
		const std::filesystem::path iconDir = Paths::EditorAssets() / "icons";
		m_ToolbarProps.PlayButtonTextureID = Application::Instance().GetAssetManager().Load<Texture2D>((iconDir / "Play.png").string())->GetID();
		m_ToolbarProps.PauseButtonTextureID = Application::Instance().GetAssetManager().Load<Texture2D>((iconDir / "Pause.png").string())->GetID();
		m_ToolbarProps.StopButtonTextureID = Application::Instance().GetAssetManager().Load<Texture2D>((iconDir / "Stop.png").string())->GetID();

		// Welcome screen shown until a project is opened
		m_WelcomeDialog.OnAttach();
		m_WelcomeDialog.SetNewProjectCallback([this]() { NewProject(); });
		m_WelcomeDialog.SetOpenProjectCallback([this](const std::string& projectFilePath) { OpenProject(projectFilePath); });

		// Double-clicking a .ebproj in Explorer hands it to us as an argument. Loading it here also
		// suppresses the welcome screen, which keys off whether a project is active.
		auto& app = Application::Instance();
		for (int i = 1; i < app.GetCommandLineArgsCount(); ++i)
		{
			const std::filesystem::path candidate = app.GetCommandLineArg(i);

			// Case-insensitively, because path comparison is not and Explorer may hand us any casing.
			if (_stricmp(candidate.extension().string().c_str(), ".ebproj") != 0)
				continue;

			// A stale shortcut or renamed project falls through to the welcome screen rather than failing.
			if (std::filesystem::exists(candidate))
				OpenProject(candidate.string());
			else
				EB_WARN("Project passed on the command line does not exist: {}", candidate.string());

			break;
		}

		// Keep m_EditorScene in sync when a deferred scene swap completes (e.g. after CreateScene queues a load)
		Application::Instance().GetSceneManager().SetOnSceneChangedCallback([this](SharedPtr<Scene> newScene)
		{
			if (m_Context.CurrentSceneState == SceneState::Edit && !m_Context.IsEditingPrefab)
				m_EditorScene = newScene;

			// Update viewport size for the new scene so render targets are correct from the start
			m_ViewportSize = { (float)m_OutputFramebuffer->GetSpecification().Width, (float)m_OutputFramebuffer->GetSpecification().Height };
			m_Camera.SetViewportSize(static_cast<uint32_t>(m_ViewportSize.x), static_cast<uint32_t>(m_ViewportSize.y));

			// De-select entity on scene change so no panel keeps an Entity wrapper from the old scene.
			ClearEntitySelectionState();
		});
	}

	void EditorLayer::OnDetach()
	{
		m_Preferences.Save();
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
		// Publish the docked viewport rect so UI hit-testing and scripts get viewport-local mouse coords.
		// Bounds come from the ImGui pass, so this is one frame behind - the same lag OnMouseClick lives with.
		//
		// Play mode sets ImGuiConfigFlags_NoMouse, which makes IsWindowHovered() report false for the
		// whole session - so m_ViewportHovered is only a meaningful gate while editing.
		Vector2f viewportExtent = m_ViewportBounds[1] - m_ViewportBounds[0];
		if (viewportExtent.x > 0.0f && viewportExtent.y > 0.0f)
		{
			// m_ViewportBounds come from ImGui, which reports desktop coordinates while
			// ImGuiConfigFlags_ViewportsEnable is set, but Input::GetMousePosition is client-relative
			// to the main window. Rebasing keeps both in the same space - without it every UI hit test
			// is offset by the window's client origin (its border and title bar).
			ImVec2 mainViewportPosition = ImGui::GetMainViewport()->Pos;
			Vector2f viewportMin = m_ViewportBounds[0] - Vector2f(mainViewportPosition.x, mainViewportPosition.y);

			bool inputActive = m_Context.CurrentSceneState == SceneState::Play || m_ViewportHovered;
			Input::SetViewportRect(viewportMin, viewportExtent, inputActive);
		}

		if (auto activeScene = m_Context.ActiveScene())
		{
			Entity selectedEntity = ResolveEntityInScene(activeScene, m_Context.SelectedEntity);
			if (selectedEntity != Constants::Entities::InvalidEntityID)
				m_Context.SelectedEntity = selectedEntity;
			else if (m_Context.SelectedEntity != Constants::Entities::InvalidEntityID)
			{
				m_Context.SelectedEntity = Entity();
				m_PreviousSelectedEntity = Entity();
			}

			Entity previousEntity = ResolveEntityInScene(activeScene, m_PreviousSelectedEntity);
			if (previousEntity != Constants::Entities::InvalidEntityID)
				m_PreviousSelectedEntity = previousEntity;
			else if (m_PreviousSelectedEntity != Constants::Entities::InvalidEntityID)
				m_PreviousSelectedEntity = Entity();
		}

		SyncEntitySelectionState();

		// Reload any user shaders whose source files have changed on disk so the user sees
		// edits live without restarting the editor.
		Application::Instance().GetAssetManager().PollShaderHotReload();

		// Disable scrolling of editor camera if the viewport isn't hovered to prevent conflicts with scrollable panels
		if (!m_ViewportHovered)
			m_Camera.SetScrollDisabled(true);
		else
			m_Camera.SetScrollDisabled(false);

		// Update the panels
		for (auto& panel : m_Panels)
			panel->OnUpdate(delta);

		// Update viewport tabs
		m_ViewportTabs.OnUpdate(delta, this);

		if (auto activeScene = m_Context.ActiveScene())
		{
			m_OutputFramebuffer->Bind();

			RenderAction::SetViewport(0, 0, m_OutputFramebuffer->GetSpecification().Width, m_OutputFramebuffer->GetSpecification().Height);

			// Scene state render
			switch (m_Context.CurrentSceneState)
			{
				case SceneState::Edit:
				{
					SyncEditorIconComponents(activeScene.Ptr());

					m_Camera.OnUpdate(delta);
					m_EditorRenderPassSettings.CameraTransform = Math::Inverse(m_Camera.GetViewMatrix());
					m_EditorRenderPassSettings.DrawHUD = m_DrawAllHUD;
					m_EditorRenderPassSettings.SelectedEntity = m_Context.SelectedEntity.GetEntityHandle();
					activeScene->OnUpdateEdit(delta, m_EditorRenderPassSettings);
					break;
				}
				case SceneState::Play:
				{
					try
					{
						activeScene->OnUpdateRuntime(delta);
					}
					catch (const std::exception& e)
					{
						StopRuntimeAfterError(e.what());
					}
					catch (...)
					{
						StopRuntimeAfterError("Unknown runtime error");
					}
					break;
				}
				case SceneState::Pause:
				{
					activeScene->OnUpdateRuntime(0.0f);
					break;
				}
				default:
					EB_CORE_ASSERT(false, "Unhandled scene state!");
			}

			if (m_Context.SelectedEntity != Constants::Entities::InvalidEntityID && ResolveEntityInScene(activeScene, m_Context.SelectedEntity) == Constants::Entities::InvalidEntityID)
			{
				m_Context.SelectedEntity = Entity();
				m_PreviousSelectedEntity = Entity();
			}

			m_OutputFramebuffer->Unbind();

			// Camera preview render pass (if applicable)
			if (m_Context.CurrentSceneState == SceneState::Edit
				&& m_Context.SelectedEntity != Constants::Entities::InvalidEntityID 
				&& m_Context.SelectedEntity.ContainsComponent<CameraComponent>())
			{
				auto& cameraComp = m_Context.SelectedEntity.GetComponent<CameraComponent>();
				auto& transform = m_Context.SelectedEntity.GetComponent<TransformComponent>();

				// Render the preview at the main viewport's resolution so the scene's internal
				// render pass FBOs (G-buffer, lighting, post-process, etc.) don't have to be
				// reallocated each frame. ImGui scales the resulting texture down for display.
				uint32_t mainW = m_OutputFramebuffer->GetSpecification().Width;
				uint32_t mainH = m_OutputFramebuffer->GetSpecification().Height;
				cameraComp.Camera.SetViewportSize(mainW, mainH);

				RenderPassSettings previewSettings = {
					.ActiveCamera = &cameraComp.Camera,
					.CameraTransform = transform.WorldTransform,
					.RenderMask = cameraComp.RenderMask,
					.VolumeMask = cameraComp.VolumeMask
				};

				m_CameraPreviewFramebuffer->Bind();
				RenderAction::SetViewport(0, 0, mainW, mainH);
				activeScene->OnUpdateEdit(delta, previewSettings);
				m_CameraPreviewFramebuffer->Unbind();
			}
		}

		// Set cursor locking
		if (m_Context.CurrentSceneState == SceneState::Play)
		{
			if (Input::IsKeyPressed(KeyCode::Escape))
			{
				Input::SetCursorMode(CursorMode::Normal);
				Input::SetMousePosition(m_ViewportBounds[0].x + m_ViewportSize.x / 2.0f, m_ViewportBounds[0].y + m_ViewportSize.y / 2.0f);
				ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
				m_Context.CurrentSceneState = SceneState::Pause;
			}

			// Clicking a UI element must not grab the cursor - a locked cursor reports unbounded
			// virtual coordinates, which would make every button in the game unclickable.
			auto uiInputSystem = Application::Instance().GetSystemManager().GetSystem<UIInputSystem>();
			bool pointerOverUI = uiInputSystem && uiInputSystem->IsPointerOverUI();

			if (m_ViewportHovered && !pointerOverUI && Input::IsMouseButtonPressed(MouseButton::Left))
			{
				Input::SetCursorMode(CursorMode::Locked);
				Input::SetMousePosition(m_ViewportBounds[0].x + m_ViewportSize.x / 2.0f, m_ViewportBounds[0].y + m_ViewportSize.y / 2.0f);
				ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
			}
		}

		RenderAction::SetClearColor(Vector4f(0.0f, 0.0f, 0.0f, 1.0f));
		RenderAction::Clear(RendererAPI::RenderBit::Color);
	}

	void EditorLayer::SyncEditorIconComponents(Scene* scene)
	{
		if (!scene)
			return;

		auto& registry = scene->GetRegistry();
		std::vector<EntityID> entityIds;
		entityIds.reserve(256);

		for (EntityID entityID : registry.Query<IDComponent>())
			entityIds.push_back(entityID);

		for (EntityID entityID : entityIds)
		{
			UUID iconTexture = Constants::InvalidUUID;
			float iconSize = 1.0f;

			if (registry.ContainsComponent<CameraComponent>(entityID))
			{
				iconTexture = EditorConstants::Assets::CameraTexUUID;
			}
			else if (registry.ContainsComponent<DirectionalLightComponent>(entityID))
			{
				iconTexture = EditorConstants::Assets::DirectionalLightTexUUID;
				iconSize = 1.5f;
			}
			else if (registry.ContainsComponent<SpotLightComponent>(entityID))
			{
				iconTexture = EditorConstants::Assets::SpotLightTexUUID;
			}
			else if (registry.ContainsComponent<PointLightComponent>(entityID))
			{
				iconTexture = EditorConstants::Assets::PointLightTexUUID;
			}
			else if (registry.ContainsComponent<AudioSourceComponent>(entityID))
			{
				//iconTexture = EditorConstants::Assets::AudioSourceTexUUID;
				iconTexture = Application::Instance().GetAssetManager().ContainsAssetWithName(EditorConstants::Assets::AudioSourceTex)
					? EditorConstants::Assets::AudioSourceTexUUID
					: EditorConstants::Assets::PointLightTexUUID;
			}

			Entity entity(entityID, scene);
			if (iconTexture != Constants::InvalidUUID)
			{
				auto& icon = entity.ContainsComponent<EditorIconComponent>()
					? entity.GetComponent<EditorIconComponent>()
					: entity.AttachComponent<EditorIconComponent>();

				icon.TextureHandle = iconTexture;
				icon.Tint = Vector4f(1.0f);
				icon.Spherical = true;
				icon.StaticSize = true;
				icon.Size = iconSize;
			}
			else if (entity.ContainsComponent<EditorIconComponent>())
			{
				entity.DetachComponent<EditorIconComponent>();
			}
		}
	}

	void EditorLayer::OnImGuiRender(TimeStep delta)
	{
		ImGuizmo::BeginFrame();

		// The welcome screen paints its own backdrop, so the dockspace is only kept alive (not drawn)
		// while no project is loaded - otherwise it would cover the backdrop with an empty grey node.
		const bool hasProject = ProjectManager::GetActive() != nullptr;
		ImGuiDockNodeFlags dockFlags = hasProject
			? ImGuiDockNodeFlags_None
			: ImGuiDockNodeFlags_KeepAliveOnly | ImGuiDockNodeFlags_PassthruCentralNode;
		ImGui::DockSpaceOverViewport(0, nullptr, dockFlags);

		// Block all editor interaction until the user opens or creates a project
		if (!hasProject)
		{
			// Hand the screen over to the New Project dialog once it is queued instead of fighting it for focus
			bool newProjectPending = m_NewProjectSettings.ShowProjectSettingsPopup || ImGui::IsPopupOpen("New Project");
			m_WelcomeDialog.OnImGuiRender(!newProjectPending);

			RenderNewProjectPopup();
			return;
		}

		//ImGui::ShowDemoWindow();

		// Menu Bar
		RenderMenuBar();
		if (m_SkipSceneUiThisFrame)
		{
			m_SkipSceneUiThisFrame = false;
			RenderNewScenePopup();
			RenderNewProjectPopup();
			return;
		}

		DrawToolbar();
		RenderSceneViewport();
		RenderClosePrefabPrompt();

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

		HandleSceneOpenRequest();
		HandlePrefabOpenRequest();
		HandleAnimationOpenRequest();
		HandleSkeletonMaskOpenRequest();

		// Pop up for new project
		RenderNewScenePopup();
		RenderNewProjectPopup();

		// Deferred removal - entities/components are queued during iteration and removed at frame end
		RemovePendingComponents();
		RemovePendingEntities();
	}

	void EditorLayer::LoadDefaultAssets()
	{
		auto& assetManager = Application::Instance().GetAssetManager();

		// Textures
		auto pointLightTex = assetManager.Load<Texture2D>(EditorConstants::Assets::PointLightTexUUID, EditorConstants::Assets::PointLightTex, (assetManager.GetProjectAssetDirectory() / "icons/PointLight.png").string());
		auto directionalLightTex = assetManager.Load<Texture2D>(EditorConstants::Assets::DirectionalLightTexUUID, EditorConstants::Assets::DirectionalLightTex, (assetManager.GetProjectAssetDirectory() / "icons/DirectionalLight.png").string());
		auto spotLightTex = assetManager.Load<Texture2D>(EditorConstants::Assets::SpotLightTexUUID, EditorConstants::Assets::SpotLightTex, (assetManager.GetProjectAssetDirectory() / "icons/SpotLight.png").string());
		auto cameraTex = assetManager.Load<Texture2D>(EditorConstants::Assets::CameraTexUUID, EditorConstants::Assets::CameraTex, (assetManager.GetProjectAssetDirectory() / "icons/Camera.png").string());
		//auto audioSourceTex = assetManager.Load<Texture2D>(EditorConstants::Assets::AudioSourceTexUUID, EditorConstants::Assets::AudioSourceTex, (assetManager.GetProjectAssetDirectory() / "icons/AudioSource.png").string());

		auto audioSourceIconPath = assetManager.GetProjectAssetDirectory() / "icons/AudioSource.png";
		if (std::filesystem::exists(audioSourceIconPath))
		{
			auto audioSourceTex = assetManager.Load<Texture2D>(EditorConstants::Assets::AudioSourceTexUUID, EditorConstants::Assets::AudioSourceTex, audioSourceIconPath.string());
		}
	}

	void EditorLayer::OnRuntimeStart()
	{
		if (m_Context.IsEditingPrefab)
		{
			auto evt = UINotificationEvent("Close prefab edit mode before entering Play mode.", UINotificationEvent::Severity::Warning);
			m_Context.EventCallback(evt);
			return;
		}

		if (!m_EditorScene)
		{
			auto evt = UINotificationEvent("Open a scene tab before entering Play mode.", UINotificationEvent::Severity::Warning);
			m_Context.EventCallback(evt);
			return;
		}

		try
		{
			auto& sceneManager = Application::Instance().GetSceneManager();

			UUID selectedUUID = Constants::InvalidUUID;
			if (m_Context.SelectedEntity != Constants::Entities::InvalidEntityID)
			{
				selectedUUID = m_Context.SelectedEntity.GetUUID();

				auto queueOutlineRemoval = [this](Entity entity)
				{
					if (entity != Constants::Entities::InvalidEntityID && entity.ContainsComponent<OutlineComponent>())
						RemoveComponentFromEntity<OutlineComponent>(entity);
				};

				queueOutlineRemoval(m_Context.SelectedEntity);
				if (m_Context.SelectedEntity.IsRootParent())
				{
					for (auto& child : m_Context.SelectedEntity.GetAllChildren())
						queueOutlineRemoval(child);
				}
			}

			RemovePendingComponents();
			m_Context.SelectedEntity = Entity();
			m_PreviousSelectedEntity = Entity();

			// Create a deep copy of the editor scene for runtime so the editor copy is never mutated
			auto runtimeScene = Scene::CopyScene(m_EditorScene);
			runtimeScene->OnViewportResize(static_cast<uint32_t>(m_ViewportSize.x), static_cast<uint32_t>(m_ViewportSize.y));
			sceneManager.SetActiveScene(runtimeScene); // OnAttach is called inside SetActiveScene
			m_Context.CurrentSceneState = SceneState::Play;

			if (selectedUUID != Constants::InvalidUUID)
			{
				Entity runtimeEntity = runtimeScene->GetEntity(selectedUUID);
				if (runtimeEntity != Constants::Entities::InvalidEntityID)
					m_Context.SelectedEntity = runtimeEntity;
			}
			m_PreviousSelectedEntity = m_Context.SelectedEntity;

			sceneManager.GetActiveScene()->OnRuntimeStart();

			Input::SetCursorMode(CursorMode::Locked);
			Input::SetMousePosition({ m_ViewportBounds[0].x + m_ViewportSize.x / 2.0f, m_ViewportBounds[0].y + m_ViewportSize.y / 2.0f });
			ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;

			ProjectManager::GetActive()->ResetSceneIndex();
		}
		catch (const std::exception& e)
		{
			StopRuntimeAfterError(e.what());
		}
		catch (...)
		{
			StopRuntimeAfterError("Unknown runtime startup error");
		}
	}

	void EditorLayer::OnRuntimeStop()
	{
		auto& sceneManager = Application::Instance().GetSceneManager();
		if (auto activeScene = sceneManager.GetActiveScene())
			activeScene->OnRuntimeStop();

		RemovePendingComponents();

		// Capture the selected entity's UUID before clearing the selection so we can
		// re-select the corresponding entity in the editor scene after the runtime
		// scene (and its registry) is destroyed. Without this, m_Context.SelectedEntity
		// holds a dangling m_SceneHandle pointer into the now-dead runtime scene.
		UUID selectedUUID = Constants::InvalidUUID;
		if (m_Context.SelectedEntity != Constants::Entities::InvalidEntityID)
			selectedUUID = m_Context.SelectedEntity.GetUUID();
		m_Context.SelectedEntity = Entity();
		m_PreviousSelectedEntity = Entity();

		// Restore the editor scene as the active scene
		sceneManager.SetActiveScene(m_EditorScene); // OnDetach on runtime copy, OnAttach on editor scene
		sceneManager.GetActiveScene()->OnViewportResize(static_cast<uint32_t>(m_ViewportSize.x), static_cast<uint32_t>(m_ViewportSize.y));

		// OnSceneAttach was called for the runtime scene copy during OnRuntimeStart, which
		// called RestartPhysicsWorld and wiped the RP3D world that the editor scene's bodies
		// lived in. Re-attach the physics system to the editor scene so it rebuilds a fresh
		// world and recreates all editor bodies via ConnectAndRetroact.
		auto& systemManager = Application::Instance().GetSystemManager();
		systemManager.GetSystem<PhysicsSystem>()->OnSceneAttach(m_EditorScene.Ptr());

		m_Context.CurrentSceneState = SceneState::Edit;

		// Re-select the previously selected entity using its UUID now that the editor
		// scene's registry is live again.
		if (selectedUUID != Constants::InvalidUUID)
		{
			Entity editorEntity = m_EditorScene->GetEntity(selectedUUID);
			if (editorEntity != Constants::Entities::InvalidEntityID)
				m_Context.SelectedEntity = editorEntity;
		}

		Input::SetCursorMode(CursorMode::Normal);
		ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
	}

	void EditorLayer::StopRuntimeAfterError(const std::string& message)
	{
		StopRuntimeAfterError(message.c_str());
	}

	void EditorLayer::StopRuntimeAfterError(const char* message)
	{
		EB_CORE_ERROR("Runtime error while in Play mode: {}", message ? message : "Unknown runtime error");
		EB_CORE_ERROR("Stopping Play mode and returning to Edit mode to keep the editor running.");

		auto evt = UINotificationEvent("An error occurred during runtime. Check the console or logs for more details.", UINotificationEvent::Severity::Error);
		m_Context.EventCallback(evt);

		if (m_Context.CurrentSceneState == SceneState::Play || m_Context.CurrentSceneState == SceneState::Pause)
		{
			try
			{
				OnRuntimeStop();
			}
			catch (const std::exception& e)
			{
				EB_CORE_ERROR("Error while stopping runtime after failure: {}", e.what());
				m_Context.CurrentSceneState = SceneState::Edit;
				Application::Instance().GetSceneManager().SetActiveScene(m_EditorScene);
				Input::SetCursorMode(CursorMode::Normal);
				ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
			}
			catch (...)
			{
				EB_CORE_ERROR("Unknown error while stopping runtime after failure.");
				m_Context.CurrentSceneState = SceneState::Edit;
				Application::Instance().GetSceneManager().SetActiveScene(m_EditorScene);
				Input::SetCursorMode(CursorMode::Normal);
				ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
			}
		}
		else
		{
			m_Context.CurrentSceneState = SceneState::Edit;
			Input::SetCursorMode(CursorMode::Normal);
			ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
		}
	}

	void EditorLayer::RenderMenuBar()
	{
		ImGui::BeginMainMenuBar();
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6, 6));
		if (ImGui::BeginMenu("File"))
		{
			bool projectExists = ProjectManager::GetActive() != nullptr;
			bool editingPrefab = m_Context.IsEditingPrefab;
			if (ImGui::MenuItem("New Project", "Ctrl+Shift+N"))
			{
				NewProject();
			}
			if (ImGui::MenuItem("Open Project", "Ctrl+Shift+O"))
			{
				OpenProject();
			}
			if (ImGui::MenuItem("Save Project", "Ctrl+Shift+S", false, projectExists))
			{
				SaveProject();
			}
			if (ImGui::MenuItem("Export Project", "Ctrl+Shift+E", nullptr, projectExists))
			{
				// Make sure everything is saved before exporting!
				SaveProject();

				// Ask the user where they want to save the game
				std::string exportDir = FileDialog::OpenDirectory();
				if (!exportDir.empty())
				{
					if (ProjectManager::ExportActiveProject(std::filesystem::path(exportDir)))
					{
						auto evt = UINotificationEvent("Project exported successfully!");
						m_Context.EventCallback(evt);
					}
					else
					{
						auto evt = UINotificationEvent("Project export failed!", UINotificationEvent::Severity::Error);
						m_Context.EventCallback(evt);
					}
				}
			}

			ImGui::Separator();

			if (ImGui::MenuItem("New Scene", "Ctrl+N", false, projectExists && m_Context.CurrentSceneState == SceneState::Edit))
			{
				NewScene();
			}

			if (ImGui::MenuItem("Open Scene", "Ctrl+O", false, projectExists && m_Context.CurrentSceneState == SceneState::Edit))
			{
				OpenScene();
			}

			if (ImGui::MenuItem("Open Prefab", nullptr, false, projectExists && m_Context.CurrentSceneState == SceneState::Edit))
			{
				OpenPrefab();
			}

			if (ImGui::MenuItem(editingPrefab ? "Save Prefab" : "Save Scene", "Ctrl+S", false, projectExists))
			{
				if (editingPrefab)
					SaveOpenPrefab();
				else
					SaveScene(false);
			}

			if (ImGui::MenuItem("Save Scene As", nullptr, false, projectExists && !editingPrefab))
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

				if (auto logPanel = GetPanel<LogPanel>())
					ImGui::MenuItem("Log", nullptr, &logPanel->Open);

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Debug"))
			{
				DrawDebugDrawToggles();
				ImGui::EndMenu();
			}

			ImGui::EndMenu();
		}

		ImGui::PopStyleVar();
		ImGui::EndMainMenuBar();
	}

	void EditorLayer::RenderSceneViewport()
	{
		bool renderedActiveTab = m_ViewportTabs.Render(
			this,
			[this](size_t previousViewerIndex, size_t activeViewerIndex, EditorViewportViewer& activeViewer) { OnViewportViewerActivated(previousViewerIndex, activeViewerIndex, activeViewer); },
			[this](size_t viewerIndex, EditorViewportViewer& viewer, bool saveBeforeClose) { return OnViewportViewerCloseRequested(viewerIndex, viewer, saveBeforeClose); });

		if (!renderedActiveTab)
		{
			m_ViewportHovered = false;
			m_ViewportFocused = false;
		}
	}

	void EditorLayer::RenderClosePrefabPrompt()
	{
		const char* popupName = "Close Prefab";
		if (m_ShowClosePrefabPrompt)
		{
			ImGui::OpenPopup(popupName);
			m_ShowClosePrefabPrompt = false;
		}

		ImGui::SetNextWindowSize(ImVec2(360.0f, 0.0f), ImGuiCond_Appearing);
		if (!ImGui::BeginPopupModal(popupName, nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize))
			return;

		EditorViewportViewer* viewer = m_PendingPrefabCloseViewerIndex >= 0
			? m_ViewportTabs.GetViewer(static_cast<size_t>(m_PendingPrefabCloseViewerIndex))
			: nullptr;

		if (!viewer || viewer->GetType() != EditorViewportViewer::Type::Prefab)
		{
			m_PendingPrefabCloseViewerIndex = -1;
			ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
			return;
		}

		std::string prefabName = std::filesystem::path(viewer->GetFilePath()).filename().string();
		if (prefabName.empty())
			prefabName = viewer->GetTitle();

		ImGui::TextWrapped("Save changes to %s before closing?", prefabName.c_str());
		ImGui::Spacing();

		int viewerIndex = m_PendingPrefabCloseViewerIndex;
		if (ImGui::Button("Save", ImVec2(100.0f, 0.0f)))
		{
			ActivateViewer(static_cast<size_t>(viewerIndex));
			if (SaveOpenPrefab() && CloseViewer(static_cast<size_t>(viewerIndex), false))
			{
				auto evt = UINotificationEvent("Closed prefab tab.");
				m_Context.EventCallback(evt);
				m_PendingPrefabCloseViewerIndex = -1;
				ImGui::CloseCurrentPopup();
			}
		}

		ImGui::SameLine();
		if (ImGui::Button("Discard", ImVec2(100.0f, 0.0f)))
		{
			if (CloseViewer(static_cast<size_t>(viewerIndex), false))
			{
				auto evt = UINotificationEvent("Closed prefab tab.");
				m_Context.EventCallback(evt);
				m_PendingPrefabCloseViewerIndex = -1;
				ImGui::CloseCurrentPopup();
			}
		}

		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(100.0f, 0.0f)))
		{
			m_PendingPrefabCloseViewerIndex = -1;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
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
				if (!CloseAllViewers(true))
				{
					ImGui::EndPopup();
					return;
				}

				ClearEntitySelectionState();

				std::filesystem::path projectDirPath = std::filesystem::path(m_NewProjectSettings.ProjectDirectory) / m_NewProjectSettings.ProjectName;
				std::filesystem::create_directories(projectDirPath);

				std::string projectNameFinal = m_NewProjectSettings.ProjectName + ".ebproj";
				std::filesystem::path fullProjectPath = projectDirPath / projectNameFinal;

				auto project = ProjectManager::NewProject(fullProjectPath.string());

				OpenScene(project->GetStartScenePath().string());

				if (auto assetPanel = GetPanel<AssetManagerPanel>())
				{
					assetPanel->UpdateRootDirectory(project->GetAssetDirectory().parent_path());
				}

				OnProjectOpened(project->GetProjectFilePath());

				ImGui::CloseCurrentPopup();
			}

			if (!isValid)
				ImGui::EndDisabled();

			ImGui::SameLine();

			if (ImGui::Button("Cancel", ImVec2(120, 0)))
			{
				m_NewProjectSettings.ProjectName = "";
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void EditorLayer::RenderNewScenePopup()
	{
		if (m_ShowNewScenePopup)
		{
			ImGui::OpenPopup("New Scene");
			m_ShowNewScenePopup = false;
		}

		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(325, 150), ImGuiCond_Appearing);

		if (ImGui::BeginPopupModal("New Scene", NULL, ImGuiWindowFlags_NoSavedSettings))
		{
			if (UI::PropertyGrid::Begin("NewSceneTable"))
			{
				UI::PropertyGrid::InputText("Scene Name", m_NewSceneName);

				UI::PropertyGrid::End();
			}

			ImGui::Dummy(ImVec2(0.0f, ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing()));

			// Actions
			bool isValid = !m_NewSceneName.empty();
			if (!isValid)
				ImGui::BeginDisabled();

			if (ImGui::Button("Create", ImVec2(120, 0)))
			{
				UINotificationEvent evt;
				auto& assetManager = Application::Instance().GetAssetManager();
				auto scenePath = ProjectManager::GetActive()->GetScenesDirectory() / (m_NewSceneName + ".ebs");
				auto newScene = assetManager.Load<Scene>(m_NewSceneName, scenePath.string(), false);
				SceneSerializer serializer(newScene);

				if (newScene && serializer.Serialize(scenePath.string()))
				{
					ProjectManager::GetActive()->AddSceneToBuild(newScene->GetUUID());
					OpenScene(newScene->GetFilePath());
					evt = UINotificationEvent("New Scene created!");
				}
				else
					evt = UINotificationEvent("Failed to create scene!", UINotificationEvent::Severity::Error);

				m_Context.EventCallback(evt);

				ImGui::CloseCurrentPopup();
			}

			if (!isValid)
				ImGui::EndDisabled();

			ImGui::SameLine();

			if (ImGui::Button("Cancel", ImVec2(120, 0)))
			{
				m_NewSceneName = "";
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

		bool isEditMode = m_Context.CurrentSceneState == SceneState::Edit;
		bool activeProject = ProjectManager::GetActive() != nullptr;
		bool control = Input::IsKeyPressed(KeyCode::LeftControl) || Input::IsKeyPressed(KeyCode::RightControl);
		bool shift = Input::IsKeyPressed(KeyCode::LeftShift) || Input::IsKeyPressed(KeyCode::RightShift);

		KeyCode key = e.GetKeyCode();
		switch (key)
		{
			// Gizmos (Translate, Rotate, Scale) -> W, E, R (Q to disable) (T for all)
			case KeyCode::Q:
				if(isEditMode) m_GizmoType = -1;
				break;
			case KeyCode::W:
				if (isEditMode) m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
				break;
			case KeyCode::E:
				if (isEditMode) m_GizmoType = ImGuizmo::OPERATION::ROTATE;
				break;
			case KeyCode::R:
				if (isEditMode) m_GizmoType = ImGuizmo::OPERATION::SCALE;
				break;
			case KeyCode::T:
				if (isEditMode) m_GizmoType = ImGuizmo::OPERATION::UNIVERSAL;
				break;

			// Frame the selection; Ctrl+F is left free for a future search.
			case KeyCode::F:
				if (isEditMode && !control)
					FocusSelection();
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
				{
					SaveProject();
				}
				else if (activeProject && control)
				{
					auto viewerType = m_Context.ActiveViewportViewer ? m_Context.ActiveViewportViewer->GetType() : EditorViewportViewer::Type::None;
					if (viewerType == EditorViewportViewer::Type::Prefab)
						SaveOpenPrefab();
					else if (viewerType == EditorViewportViewer::Type::Animation)
						static_cast<AnimationViewportViewer*>(m_Context.ActiveViewportViewer)->SaveAnimationStateMachine(this);
					else if (viewerType == EditorViewportViewer::Type::SkeletonMask)
						static_cast<SkeletonMaskViewportViewer*>(m_Context.ActiveViewportViewer)->SaveSkeletonMask(this);
					else
						SaveScene(false);
				}
				break;

			case KeyCode::Escape:
				break;

			// Entity Hot keys
			case KeyCode::Delete:
				if (m_Context.SelectedEntity != Constants::Entities::InvalidEntityID)
					RemoveEntity(m_Context.SelectedEntity);
				break;

			case KeyCode::Enter:
				if (control)
				{
					if (m_Context.CurrentSceneState == SceneState::Play)
						OnRuntimeStop();
					else if (m_EditorScene && !m_Context.IsEditingPrefab)
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
			bool isGizmoDrawn = m_Context.SelectedEntity != Constants::Entities::InvalidEntityID && m_GizmoType != -1;
			if (isGizmoDrawn && (ImGuizmo::IsOver() || m_ViewportGizmos.IsHovered()))
				return false;

			int mouseX = 0;
			int mouseY = 0;
			if (TryGetViewportPixel(mouseX, mouseY))
			{
				if (auto activeScene = m_Context.ActiveScene())
				{
					Entity selected = activeScene->GetEntityAtPixel(mouseX, mouseY);
					m_Context.SelectedEntity = selected;
				}
			}
		}

		return false;
	}

	bool EditorLayer::TryGetViewportPixel(int& outX, int& outY) const
	{
		auto [mx, my] = ImGui::GetMousePos();

		// Convert screen-space mouse coords to viewport-local with Y flipped for OpenGL
		mx -= m_ViewportBounds[0].x;
		my -= m_ViewportBounds[0].y;

		Vector2f viewportSize = m_ViewportBounds[1] - m_ViewportBounds[0];
		my = viewportSize.y - my;

		outX = (int)mx;
		outY = (int)my;

		return outX >= 0 && outY >= 0 && outX < (int)viewportSize.x && outY < (int)viewportSize.y;
	}

	Vector3f EditorLayer::GetSpawnPosition()
	{
		Vector3f focalPoint = m_Camera.GetFocalPoint();
		if (!m_Preferences.SpawnAtCursor)
			return focalPoint;

		auto activeScene = m_Context.ActiveScene();

		int mouseX = 0;
		int mouseY = 0;
		if (activeScene && m_ViewportHovered && TryGetViewportPixel(mouseX, mouseY))
		{
			// The G-Buffer holds last frame's render, so this lands on whatever was drawn there.
			Vector3f surfacePosition;
			if (activeScene->GetWorldPositionAtPixel(mouseX, mouseY, surfacePosition))
			{
				return m_Preferences.SnapEnabled
					? Math::Snap(surfacePosition, m_Preferences.TranslateSnap)
					: surfacePosition;
			}
		}

		// Nothing under the cursor, so drop it in front of the camera rather than at the origin.
		return focalPoint;
	}

	void EditorLayer::SyncEntitySelectionState()
	{
		if (m_Context.CurrentSceneState != SceneState::Edit)
		{
			m_PreviousSelectedEntity = m_Context.SelectedEntity;
			return;
		}

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
					if (child != Constants::Entities::InvalidEntityID && child.ContainsComponent<OutlineComponent>())
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

		// TODO: Move these system debug draw code blocks to own methods

		// Automatically show debug draw lines for the selected entity if any of its colliders
		// have PreviewCollider enabled, without requiring the global physics debug draw toggle.
		auto physicsSystem = Application::Instance().GetSystemManager().GetSystem<PhysicsSystem>();
		if (physicsSystem)
		{
			auto& selected = m_Context.SelectedEntity;
			bool hasPreview = selected != Constants::Entities::InvalidEntityID &&
				((selected.ContainsComponent<BoxColliderComponent>()         && selected.GetComponent<BoxColliderComponent>().PreviewCollider)         ||
				 (selected.ContainsComponent<SphereColliderComponent>()      && selected.GetComponent<SphereColliderComponent>().PreviewCollider)      ||
				 (selected.ContainsComponent<CapsuleColliderComponent>()     && selected.GetComponent<CapsuleColliderComponent>().PreviewCollider)     ||
				 (selected.ContainsComponent<ConvexMeshColliderComponent>()  && selected.GetComponent<ConvexMeshColliderComponent>().PreviewCollider)  ||
				 (selected.ContainsComponent<ConcaveMeshColliderComponent>() && selected.GetComponent<ConcaveMeshColliderComponent>().PreviewCollider));

			if (hasPreview)
				physicsSystem->SetColliderPreviewEntity(selected.GetEntityHandle());
			else
				physicsSystem->ClearColliderPreviewEntity();
		}

		// Set AI preview path entity
		auto aiSystem = Application::Instance().GetSystemManager().GetSystem<AISystem>();
		if (aiSystem)
		{
			auto& selected = m_Context.SelectedEntity;
			bool hasPreview = selected != Constants::Entities::InvalidEntityID 
				&& (selected.ContainsComponent<AIAgentComponent>() || selected.ContainsComponent<WaypointComponent>() || selected.ContainsComponent<NavigationGridComponent>());

			if (hasPreview)
				aiSystem->SetPreviewEntity(selected.GetEntityHandle());
			else
				aiSystem->ClearPreviewEntity();
		}

		// Remove reset animation pose changes on the selected entity
		if (m_PreviousSelectedEntity != Constants::Entities::InvalidEntityID && m_PreviousSelectedEntity.ContainsComponent<AnimatorComponent>())
		{
			auto animSystem = Application::Instance().GetSystem<AnimationSystem>();
			animSystem->SetAnimationToTimestamp(m_Context.ActiveScene().Ptr(), Constants::InvalidUUID, m_PreviousSelectedEntity, 0.0f);
		}

		m_PreviousSelectedEntity = m_Context.SelectedEntity;
	}

	void EditorLayer::FocusSelection()
	{
		auto activeScene = m_Context.ActiveScene();
		if (!activeScene || !m_Context.SelectedEntity.IsValid())
			return;

		AABB merged{ Vector3f(std::numeric_limits<float>::max()), Vector3f(std::numeric_limits<float>::lowest()) };
		bool hasBounds = false;

		// Walk the whole subtree so framing a model root frames the model, not its empty pivot.
		std::vector<Entity> pending{ m_Context.SelectedEntity };
		while (!pending.empty())
		{
			Entity entity = pending.back();
			pending.pop_back();

			for (Entity child : entity.GetAllChildren())
				pending.push_back(child);

			AABB entityAABB;
			if (VisibilitySystem::TryGetRenderableAABB(activeScene.Ptr(), entity.GetEntityHandle(), entityAABB))
			{
				merged.WorldMin = Math::Min(merged.WorldMin, entityAABB.WorldMin);
				merged.WorldMax = Math::Max(merged.WorldMax, entityAABB.WorldMax);
				hasBounds = true;
			}
		}

		if (hasBounds)
		{
			m_Camera.FocusOn(merged);
			return;
		}

		// Lights, cameras and empty pivots draw nothing, so frame a small volume at their origin.
		if (m_Context.SelectedEntity.ContainsComponent<TransformComponent>())
		{
			Vector3f position = Vector3f(m_Context.SelectedEntity.GetComponent<TransformComponent>().WorldTransform[3]);
			m_Camera.FocusOn(position, 1.0f);
		}
	}

	// A snap-increment dropdown offering the common values plus a free-entry field.
	static bool DrawSnapPresetCombo(const char* id, const char* format, float& value,
		const float* presets, int presetCount, float minValue, float maxValue)
	{
		bool changed = false;

		char label[32];
		snprintf(label, sizeof(label), format, value);

		ImGui::SetNextItemWidth(80.0f);
		if (ImGui::BeginCombo(id, label))
		{
			for (int i = 0; i < presetCount; ++i)
			{
				char presetLabel[32];
				snprintf(presetLabel, sizeof(presetLabel), format, presets[i]);
				if (ImGui::Selectable(presetLabel, value == presets[i]))
				{
					value = presets[i];
					changed = true;
				}
			}

			ImGui::Separator();
			ImGui::SetNextItemWidth(100.0f);
			if (ImGui::DragFloat("##Custom", &value, 0.01f, minValue, maxValue, format))
				changed = true;

			ImGui::EndCombo();
		}

		return changed;
	}

	// Every editor toggle lives behind one dropdown so the toolbar stays readable as more are added.
	void EditorLayer::DrawGizmoSettingsPopup()
	{
		static constexpr float translatePresets[] = { 0.05f, 0.1f, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f };
		static constexpr float rotatePresets[] = { 5.0f, 15.0f, 45.0f, 90.0f };

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 2));

		// The label carries the snap state so the common setting is readable without opening it.
		char buttonLabel[64];
		if (m_Preferences.SnapEnabled)
			snprintf(buttonLabel, sizeof(buttonLabel), "Gizmos  %.4g / %.0f\xc2\xb0", m_Preferences.TranslateSnap, m_Preferences.RotateSnap);
		else
			snprintf(buttonLabel, sizeof(buttonLabel), "Gizmos  (no snap)");

		if (ImGui::Button(buttonLabel))
			ImGui::OpenPopup("GizmoSettingsPopup");

		ImGui::PopStyleVar();

		if (!ImGui::BeginPopup("GizmoSettingsPopup"))
			return;

		bool changed = false;

		ImGui::SeparatorText("Snapping");

		changed |= ImGui::Checkbox("Enable snapping", &m_Preferences.SnapEnabled);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Hold Ctrl while dragging a gizmo to invert this.");

		ImGui::Text("Grid");
		ImGui::SameLine(90.0f);
		changed |= DrawSnapPresetCombo("##GridSnap", "%.4g", m_Preferences.TranslateSnap,
			translatePresets, IM_ARRAYSIZE(translatePresets), 0.001f, 100.0f);

		ImGui::Text("Angle");
		ImGui::SameLine(90.0f);
		changed |= DrawSnapPresetCombo("##AngleSnap", "%.0f\xc2\xb0", m_Preferences.RotateSnap,
			rotatePresets, IM_ARRAYSIZE(rotatePresets), 1.0f, 180.0f);

		ImGui::SeparatorText("Gizmo");

		ImGui::Text("Space");
		ImGui::SameLine(90.0f);
		ImGui::SetNextItemWidth(120.0f);
		if (ImGui::BeginCombo("##GizmoSpace", m_Preferences.GizmoLocalSpace ? "Local" : "World"))
		{
			if (ImGui::Selectable("World", !m_Preferences.GizmoLocalSpace))
			{
				m_Preferences.GizmoLocalSpace = false;
				changed = true;
			}
			if (ImGui::Selectable("Local", m_Preferences.GizmoLocalSpace))
			{
				m_Preferences.GizmoLocalSpace = true;
				changed = true;
			}
			ImGui::EndCombo();
		}

		ImGui::SeparatorText("Placement");

		changed |= ImGui::Checkbox("Spawn new entities at cursor", &m_Preferences.SpawnAtCursor);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Off places them at the camera focal point instead.");

		ImGui::SeparatorText("Display");

		ImGui::Checkbox("Draw all HUD icons", &m_DrawAllHUD);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Off draws icons only for the selected entity.");

		DrawDebugDrawToggles();

		ImGui::EndPopup();

		// Preferences are small and change rarely, so write them out immediately rather than
		// relying on a clean shutdown.
		if (changed)
			m_Preferences.Save();
	}

	// Shared by the Gizmos dropdown and the Editor > Debug menu; both drive the same system state.
	void EditorLayer::DrawDebugDrawToggles()
	{
		ImGui::SeparatorText("Debug Draw");

		auto physicsSystem = Application::Instance().GetSystemManager().GetSystem<PhysicsSystem>();
		if (physicsSystem)
		{
			auto& debugSettings = physicsSystem->GetDebugRenderSettings();
			ImGui::Checkbox("Physics Colliders", &debugSettings.Enabled);

			if (debugSettings.Enabled)
			{
				ImGui::Indent();
				ImGui::Checkbox("Shapes", &debugSettings.DrawColliders);
				ImGui::Checkbox("Contact Points", &debugSettings.DrawContactPoints);
				ImGui::Checkbox("AABBs", &debugSettings.DrawColliderAxes);
				ImGui::Unindent();
			}
		}

		auto aiSystem = Application::Instance().GetSystemManager().GetSystem<AISystem>();
		if (aiSystem)
			ImGui::Checkbox("AI Paths", &aiSystem->GetDebugRenderSettings().Enabled);

		bool drawSelectedNavMesh = ActiveNavMeshRenderer::GetEnabled();
		if (ImGui::Checkbox("Selected NavMesh", &drawSelectedNavMesh))
			ActiveNavMeshRenderer::SetEnabled(drawSelectedNavMesh);
	}

	void EditorLayer::DrawToolbar()
	{
		// Prevent the user from resizing the dock node itself
		ImGuiWindowClass window_class;
		window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_NoResize;
		ImGui::SetNextWindowClass(&window_class);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 4)); // Added horizontal padding so it doesn't hug the absolute edge
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

		ImGui::Begin("Toolbar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

		// --- LEFT ALIGNED: VIEW CONTROLS ---
		// Add a little vertical padding so it aligns nicely with the image buttons
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);

		ImGui::Text("View");
		ImGui::SameLine();

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 2)); // Give the combo box normal padding
		ImGui::SetNextItemWidth(100.0f);

		static int currentView = 0;
		const char* views[] = { "FreeFly", "Top", "Bottom", "Left", "Right", "Front", "Back" };

		if (ImGui::BeginCombo("##CameraView", views[currentView]))
		{
			for (int i = 0; i < IM_ARRAYSIZE(views); i++)
			{
				bool isSelected = (currentView == i);
				if (ImGui::Selectable(views[i], isSelected))
				{
					currentView = i;
					m_Camera.SnapToAxis(static_cast<EditorViewDirection>(i));
				}

				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		ImGui::PopStyleVar(); // Pop the combo box padding

		ImGui::SameLine();

		DrawGizmoSettingsPopup();

		ImGui::SameLine();


		// --- CENTER ALIGNED: PLAY CONTROLS ---
		float windowWidth = ImGui::GetWindowContentRegionMax().x;
		float iconSize = 24.0f;
		float spacing = ImGui::GetStyle().ItemSpacing.x;
		float buttonSizeWithPadding = iconSize + (ImGui::GetStyle().FramePadding.x * 2.0f);

		// Calculate the width of all 3 buttons + the 2 spaces between them
		float totalGroupWidth = (buttonSizeWithPadding * 3.0f) + (spacing * 2.0f);
		float centerCursorX = (windowWidth * 0.5f) - (totalGroupWidth * 0.5f);

		// Safety check: Only push to the center if the combo box hasn't already pushed past it!
		if (centerCursorX > ImGui::GetCursorPosX())
		{
			ImGui::SetCursorPosX(centerCursorX);
		}
		else
		{
			// Just add a tiny bit of space if the window is super cramped
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
		}

		// Apply transparent/hover styling to the entire button group
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.2f, 0.5f));

		auto viewportType = m_Context.ActiveViewportViewer ? m_Context.ActiveViewportViewer->GetType() : EditorViewportViewer::Type::None;
		ImGui::BeginDisabled(m_Context.CurrentSceneState != SceneState::Edit);

		// --- PLAY STANDALONE (Orange Tint) ---
		ImVec4 orangeTint = ImVec4(0.95f, 0.47f, 0.15f, 1.00f);
		if (ImGui::ImageButton("PlayStandaloneBtn", m_ToolbarProps.PlayButtonTextureID, ImVec2(iconSize, iconSize), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), orangeTint))
		{
			SaveProject();

			std::string activeProjectPath = ProjectManager::GetActive()->GetProjectFilePath().string();

			auto absoluteRuntimePath = std::filesystem::absolute(Paths::RuntimeExe()).string();
			auto engineAssetDir = Application::Instance().GetAssetManager().GetEngineAssetDirectory();
			auto engineAssetAbsolute = std::filesystem::absolute(engineAssetDir).string();
			auto projectAssetDir = Application::Instance().GetAssetManager().GetProjectAssetDirectory().string();
			auto projectAssetAbsolute = std::filesystem::absolute(projectAssetDir).string();

			std::string command = std::format("{} \"{}\" \"{}\" \"{}\"", absoluteRuntimePath, activeProjectPath, engineAssetAbsolute, projectAssetAbsolute);
			std::thread([command]() {
				std::system(command.c_str());
				}).detach();
		}

		ImGui::EndDisabled();

		ImGui::SameLine();

		// --- PLAY / STOP EDITOR SCENE (No Tint) ---
		ImGui::BeginDisabled(viewportType != EditorViewportViewer::Type::Scene);
		if (m_Context.CurrentSceneState == SceneState::Play || m_Context.CurrentSceneState == SceneState::Pause)
		{
			if (ImGui::ImageButton("StopButton", m_ToolbarProps.StopButtonTextureID, ImVec2(iconSize, iconSize)))
			{
				OnRuntimeStop();
				m_Context.CurrentSceneState = SceneState::Edit;
			}
		}
		else
		{
			ImGui::BeginDisabled(m_Context.IsEditingPrefab);
			if (ImGui::ImageButton("PlayButton", m_ToolbarProps.PlayButtonTextureID, ImVec2(iconSize, iconSize)))
			{
				OnRuntimeStart();
			}
			ImGui::EndDisabled();
		}
		ImGui::EndDisabled();

		ImGui::SameLine();

		// --- PAUSE / RESUME (Yellow Tint) ---
		ImGui::BeginDisabled(m_Context.CurrentSceneState == SceneState::Edit);

		bool isPaused = (m_Context.CurrentSceneState == SceneState::Pause);
		ImTextureID pauseIcon = isPaused ? m_ToolbarProps.PlayButtonTextureID : m_ToolbarProps.PauseButtonTextureID;
		if (ImGui::ImageButton("PauseButton", pauseIcon, ImVec2(iconSize, iconSize)))
		{
			if (isPaused)
				m_Context.CurrentSceneState = SceneState::Play;
			else
				m_Context.CurrentSceneState = SceneState::Pause;
		}

		ImGui::EndDisabled();

		// Pop the transparent button colors
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
			if (auto activeScene = m_Context.ActiveScene())
				ImGui::Text("Entities: %d", activeScene->GetAllEntities().size());
			else
				ImGui::Text("Entities: 0");
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

	void EditorLayer::ClearEntitySelectionState()
	{
		m_Context.SelectedEntity = Entity();
		m_PreviousSelectedEntity = Entity();
		m_Context.PendingEntityRemovals.clear();
		m_Context.PendingComponentRemovals.clear();
		m_EditorRenderPassSettings.SelectedEntity = Constants::Entities::InvalidEntityID;
	}
	
	void EditorLayer::CreateEntity()
	{
		auto entity = m_Context.ActiveScene()->AddEntity("Empty_Entity");
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
		if (m_Context.IsEditingPrefab && m_Context.PendingEntityRemovals.contains(m_Context.PrefabRootEntity))
		{
			m_Context.PendingEntityRemovals.erase(m_Context.PrefabRootEntity);
			auto evt = UINotificationEvent("The prefab root cannot be deleted while editing the prefab.", UINotificationEvent::Severity::Warning);
			m_Context.EventCallback(evt);
		}

		if (m_Context.PendingEntityRemovals.contains(m_Context.SelectedEntity))
			m_Context.SelectedEntity = Entity();

		for (auto entity : m_Context.PendingEntityRemovals) {
			std::string entityName = entity.GetName();
			m_Context.ActiveScene()->RemoveEntity(entity);

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
		Entity modelEntity = m_Context.ActiveScene()->InstantiateModel(modelFilePath);
		if (m_Context.IsEditingPrefab && modelEntity != Constants::Entities::InvalidEntityID && modelEntity != m_Context.PrefabRootEntity)
			m_Context.ActiveScene()->SetEntityParent(modelEntity.GetUUID(), m_Context.PrefabRootEntity);

		// InstantiateModel takes no position, so place the root afterwards.
		if (!m_Context.IsEditingPrefab && modelEntity != Constants::Entities::InvalidEntityID
			&& modelEntity.ContainsComponent<TransformComponent>())
		{
			auto& transform = modelEntity.GetComponent<TransformComponent>();
			transform.Position = GetSpawnPosition();
			transform.InvalidateWorld();
		}

		m_Context.SelectedEntity = modelEntity;
	}

	void EditorLayer::CreateEntityFromPrefab(const std::string& prefabFilePath)
	{
		auto& assetManager = Application::Instance().GetAssetManager();
		auto prefabAsset = assetManager.Load<Prefab>(prefabFilePath);

		// A prefab edit tab has no level geometry to place against, so those keep landing on the root.
		Vector3f spawnPosition = m_Context.IsEditingPrefab ? Vector3f(0.0f) : GetSpawnPosition();
		Entity prefEntity = m_Context.IsEditingPrefab
			? m_Context.ActiveScene()->InstantiatePrefab(prefabAsset, m_Context.PrefabRootEntity, &spawnPosition)
			: m_Context.ActiveScene()->InstantiatePrefab(prefabAsset, &spawnPosition);
		m_Context.SelectedEntity = prefEntity;
	}

	void EditorLayer::OutlineEntity(Entity entity)
	{
		// If the outline was queued for removal (e.g. from deselection), cancel it instead of double-adding
		bool removed = CancelComponentRemoval<OutlineComponent>(entity);
		if (!removed)
			entity.AttachComponent<OutlineComponent>(m_OutlineEntitySelectedComp);
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

	void EditorLayer::OnProjectOpened(const std::filesystem::path& projectFilePath)
	{
		m_WelcomeDialog.UpdateRecentProjectsAndSave({ projectFilePath.stem().string(), projectFilePath.string() });
	}

	void EditorLayer::OpenProject(const std::string& projectFilePath /* = "" */)
	{
		// An empty path means the caller wants to browse for the project file
		std::string projectFile = projectFilePath.empty()
			? FileDialog::OpenFile("", "Ember Project (*.ebproj)", "*.ebproj")
			: projectFilePath;

		if (projectFile.empty())
			return;

		if (!CloseAllViewers(true))
			return;

		ClearEntitySelectionState();

		auto project = ProjectManager::LoadProject(projectFile);
		if (!project)
			return;

		m_SkipSceneUiThisFrame = true;

		// Load assets for project
		auto assetPanel = GetPanel<AssetManagerPanel>();
		if (assetPanel != nullptr) 
		{
			assetPanel->UpdateRootDirectory(project->GetAssetDirectory().parent_path());
		}

		// Load the default scene for the project (assets must be ready first)
		OpenScene(project->GetStartScenePath().string());

		OnProjectOpened(project->GetProjectFilePath());
	}

	void EditorLayer::NewScene()
	{
		m_NewSceneName = "NewScene";
		m_ShowNewScenePopup = true;
	}

	void EditorLayer::OpenScene(const std::string& scenePath /* = "" */)
	{
		if (m_Context.CurrentSceneState != SceneState::Edit)
		{
			auto evt = UINotificationEvent("Stop Play mode before opening a scene.", UINotificationEvent::Severity::Warning);
			m_Context.EventCallback(evt);
			return;
		}

		std::string sceneFile = scenePath;
		if (sceneFile.empty())
		{
			std::string sceneDirectory = ProjectManager::GetActive()->GetDefaultDirectoryForAsset(AssetType::Scene).string();
			sceneFile = FileDialog::OpenFile(sceneDirectory.c_str(), "Ember Scene (*.ebs;*.ebscene)", "*.ebs;*.ebscene");
		}

		OpenSceneViewer(sceneFile);
	}

	// Writes a scene to disk, temporarily stripping the editor-only outline components so they
	// never end up in the serialized file.
	void EditorLayer::SerializeScene(const SharedPtr<Scene>& scene, const std::string& filePath)
	{
		std::vector<Entity> outlinedEntities;
		for (Entity entity : scene->GetAllEntities())
		{
			if (entity.ContainsComponent<OutlineComponent>())
			{
				outlinedEntities.push_back(entity);
				entity.DetachComponent<OutlineComponent>();
			}
		}

		SceneSerializer serializer(scene);
		serializer.Serialize(filePath);

		for (Entity entity : outlinedEntities)
		{
			if (entity != Constants::Entities::InvalidEntityID && entity.ContainsComponent<IDComponent>())
				entity.AttachComponent<OutlineComponent>(m_OutlineEntitySelectedComp);
		}
	}

	// Saves only the scene in the active tab. Use SaveProject() to write every scene and asset.
	void EditorLayer::SaveScene(bool saveAs /* = false */)
	{
		if (m_Context.IsEditingPrefab)
		{
			SaveOpenPrefab();
			return;
		}

		auto activeScene = m_Context.ActiveScene();
		if (!activeScene)
		{
			auto evt = UINotificationEvent("No scene tab is open to save.", UINotificationEvent::Severity::Warning);
			m_Context.EventCallback(evt);
			return;
		}

		std::string scenePath = activeScene->GetFilePath();
		if (saveAs)
		{
			std::string sceneDirectory = ProjectManager::GetActive()->GetDefaultDirectoryForAsset(AssetType::Scene).string();
			scenePath = FileDialog::SaveFile(sceneDirectory.c_str(), "NewScene.ebs", "Ember Scene (*.ebs)", "*.ebs");
		}

		if (scenePath.empty())
		{
			if (!saveAs)
			{
				auto evt = UINotificationEvent("Active scene has no file path to save to.", UINotificationEvent::Severity::Warning);
				m_Context.EventCallback(evt);
			}
			return;
		}

		auto& assetManager = Application::Instance().GetAssetManager();

		// Retarget the asset before serializing so the new file is the one that actually gets written.
		if (saveAs)
		{
			std::string sceneName = std::filesystem::path(scenePath).stem().string();
			if (!assetManager.RenameAsset(activeScene->GetUUID(), sceneName, scenePath))
				activeScene->SetFilePath(std::filesystem::absolute(scenePath).string());

			if (auto* activeViewer = GetActiveViewer())
			{
				activeViewer->SetFilePath(activeScene->GetFilePath());
				activeViewer->SetTitle(EditorViewportTabs::TitleFromPath(activeScene->GetFilePath(), activeScene->GetName()));
			}

			scenePath = activeScene->GetFilePath();
		}

		SerializeScene(activeScene, scenePath);

		// The registry tracks asset paths/UUIDs, so it has to follow any change the scene made.
		std::filesystem::path assetFilePath = ProjectManager::GetActive()->GetAssetsFilePath();
		AssetRegistrySerializer assetSerializer(&assetManager);
		assetSerializer.Serialize(assetFilePath.string());

		auto evt = UINotificationEvent(std::format("Scene saved: {}", std::filesystem::path(scenePath).filename().string()));
		m_Context.EventCallback(evt);
	}

	// Saves everything the project owns: every open/loaded scene, all editable assets, the asset
	// registry and the project settings file.
	void EditorLayer::SaveProject()
	{
		if (!ProjectManager::GetActive())
		{
			auto evt = UINotificationEvent("No project is open to save.", UINotificationEvent::Severity::Warning);
			m_Context.EventCallback(evt);
			return;
		}

		// A prefab being edited lives in a throwaway scene, so it has to be saved through its own path.
		if (m_Context.IsEditingPrefab)
			SaveOpenPrefab();

		auto& assetManager = Application::Instance().GetAssetManager();

		// Serialize scenes. Every scene in the project is registered with the AssetManager, but only
		// the ones that have been opened hold entities — writing out the untouched placeholders would
		// replace their files with an empty scene.
		auto scenes = assetManager.GetAssetsOfType<Scene>();
		for (auto& scene : scenes)
		{
			if (!scene->IsEngineAsset() && !scene->GetFilePath().empty() && scene->IsLoaded())
			{
				SerializeScene(scene, scene->GetFilePath());
			}
		}

		// Serialize materials (in case their values changed)
		auto materials = assetManager.GetAssetsOfType<MaterialInstance>();
		for (auto& material : materials)
		{
			if (!material->IsEngineAsset() && !material->GetFilePath().empty())
			{
				MaterialSerializer::Serialize(material->GetFilePath(), material);
			}
		}

		// Serialize physics materials as well
		auto physicsMaterials = assetManager.GetAssetsOfType<PhysicsMaterial>();
		for (auto& physMat : physicsMaterials)
		{
			if (!physMat->IsEngineAsset() && !physMat->GetFilePath().empty())
			{
				PhysicsMaterialSerializer::Serialize(physMat->GetFilePath(), physMat);
			}
		}

		// Serialize animations
		auto animations = assetManager.GetAssetsOfType<Animation>();
		for (auto& anim : animations)
		{
			if (!anim->IsEngineAsset() && !anim->GetFilePath().empty())
			{
				AnimationSerializer::Serialize(anim->GetFilePath(), anim);
			}
		}

		// Serialize assets
		std::filesystem::path assetFilePath = ProjectManager::GetActive()->GetAssetsFilePath();
		AssetRegistrySerializer assetSerializer(&assetManager);
		assetSerializer.Serialize(assetFilePath.string());

		// Save project as well to update any project settings
		ProjectManager::SaveActiveProject();

		auto evt = UINotificationEvent(std::format("Project saved: {}", ProjectManager::GetActive()->GetConfig().ProjectName));
		m_Context.EventCallback(evt);
	}

	void EditorLayer::OpenPrefab(const std::string& prefabPath)
	{
		if (m_Context.CurrentSceneState != SceneState::Edit)
		{
			auto evt = UINotificationEvent("Stop Play mode before opening a prefab.", UINotificationEvent::Severity::Warning);
			m_Context.EventCallback(evt);
			return;
		}

		std::string prefabFile = prefabPath;
		if (prefabFile.empty())
		{
			std::string prefabDirectory = ProjectManager::GetActive()->GetDefaultDirectoryForAsset(AssetType::Prefab).string();
			prefabFile = FileDialog::OpenFile(prefabDirectory.c_str(), "Ember Prefab (*.ebprefab)", "*.ebprefab");
		}

		OpenPrefabViewer(prefabFile);
	}

	bool EditorLayer::SaveOpenPrefab()
	{
		if (!m_Context.IsEditingPrefab || !m_PrefabEditScene || !m_EditingPrefab)
			return false;

		if (m_Context.PrefabRootEntity == Constants::Entities::InvalidEntityID)
		{
			auto evt = UINotificationEvent("Cannot save prefab because the root entity is missing.", UINotificationEvent::Severity::Error);
			m_Context.EventCallback(evt);
			return false;
		}

		auto& prefabComponent = m_Context.PrefabRootEntity.ContainsComponent<PrefabComponent>()
			? m_Context.PrefabRootEntity.GetComponent<PrefabComponent>()
			: m_Context.PrefabRootEntity.AttachComponent<PrefabComponent>();
		prefabComponent.PrefabHandle = m_EditingPrefab->GetUUID();

		std::vector<Entity> outlinedEntities;
		for (Entity entity : m_PrefabEditScene->GetAllEntities())
		{
			if (entity.ContainsComponent<OutlineComponent>())
			{
				outlinedEntities.push_back(entity);
				entity.DetachComponent<OutlineComponent>();
			}
		}

		SceneSerializer serializer(m_PrefabEditScene);
		bool saved = serializer.SerializePrefab(m_Context.PrefabRootEntity, m_EditingPrefabPath);

		for (Entity entity : outlinedEntities)
		{
			if (entity != Constants::Entities::InvalidEntityID && entity.ContainsComponent<IDComponent>())
				OutlineEntity(entity);
		}

		if (!saved)
		{
			auto evt = UINotificationEvent("Failed to save prefab.", UINotificationEvent::Severity::Error);
			m_Context.EventCallback(evt);
			return false;
		}

		std::ifstream stream(m_EditingPrefabPath);
		m_EditingPrefab->YAMLData = std::string((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());

		// Prefab saves can happen while editing animation events in the scrubber panel.
		// Persist animation assets here as well so timeline event edits are not lost.
		auto& assetManager = Application::Instance().GetAssetManager();
		auto animations = assetManager.GetAssetsOfType<Animation>();
		for (auto& anim : animations)
		{
			if (!anim->IsEngineAsset() && !anim->GetFilePath().empty())
				AnimationSerializer::Serialize(anim->GetFilePath(), anim);
		}

		std::filesystem::path assetFilePath = ProjectManager::GetActive()->GetAssetDirectory() / "Assets.eba";
		AssetRegistrySerializer assetSerializer(&assetManager);
		assetSerializer.Serialize(assetFilePath.string());

		auto evt = UINotificationEvent(std::format("Prefab saved: {}", std::filesystem::path(m_EditingPrefabPath).filename().string()));
		m_Context.EventCallback(evt);
		return true;
	}

	void EditorLayer::OpenAnimationState(const std::string& path)
	{
		if (m_Context.CurrentSceneState != SceneState::Edit)
		{
			auto evt = UINotificationEvent("Stop Play mode before opening an animation controller.", UINotificationEvent::Severity::Warning);
			m_Context.EventCallback(evt);
			return;
		}

		std::string animationStateFile = path;
		if (animationStateFile.empty())
		{
			std::string animationStateDirectory = ProjectManager::GetActive()->GetDefaultDirectoryForAsset(AssetType::AnimationController).string();
			animationStateFile = FileDialog::OpenFile(animationStateDirectory.c_str(), "Ember Animation Controller (*.ebcontroller)", "*.ebcontroller");
		}

		OpenAnimationViewer(animationStateFile);
	}

	void EditorLayer::OpenSkeletonMask(const std::string& path)
	{
		if (m_Context.CurrentSceneState != SceneState::Edit)
		{
			auto evt = UINotificationEvent("Stop Play mode before opening a skeleton mask.", UINotificationEvent::Severity::Warning);
			m_Context.EventCallback(evt);
			return;
		}

		std::string skeletonMaskFile = path;
		if (skeletonMaskFile.empty())
		{
			std::string skeletonMaskDirectory = ProjectManager::GetActive()->GetDefaultDirectoryForAsset(AssetType::SkeletonMask).string();
			skeletonMaskFile = FileDialog::OpenFile(skeletonMaskDirectory.c_str(), "Ember Skeleton Mask (*.ebmask)", "*.ebmask");
		}

		OpenSkeletonMaskViewer(skeletonMaskFile);
	}

	void EditorLayer::HandlePrefabOpenRequest()
	{
		if (m_Context.RequestedPrefabOpenPath.empty())
			return;

		std::string path = m_Context.RequestedPrefabOpenPath;
		m_Context.RequestedPrefabOpenPath.clear();
		OpenPrefab(path);
	}

	void EditorLayer::HandleSceneOpenRequest()
	{
		if (m_Context.RequestedSceneOpenPath.empty())
			return;

		std::string path = m_Context.RequestedSceneOpenPath;
		m_Context.RequestedSceneOpenPath.clear();
		OpenScene(path);
	}

	void EditorLayer::HandleAnimationOpenRequest()
	{
		if (m_Context.RequestAnimationStateOpenPath.empty())
			return;

		std::string path = m_Context.RequestAnimationStateOpenPath;
		m_Context.RequestAnimationStateOpenPath.clear();
		OpenAnimationState(path);
	}

	void EditorLayer::HandleSkeletonMaskOpenRequest()
	{
		if (m_Context.RequestSkeletonMaskOpenPath.empty())
			return;

		std::string path = m_Context.RequestSkeletonMaskOpenPath;
		m_Context.RequestSkeletonMaskOpenPath.clear();
		OpenSkeletonMask(path);
	}

	EditorViewportViewer* EditorLayer::GetActiveViewer()
	{
		return m_ViewportTabs.GetActiveViewer();
	}

	const EditorViewportViewer* EditorLayer::GetActiveViewer() const
	{
		return m_ViewportTabs.GetActiveViewer();
	}

	void EditorLayer::OnViewportViewerActivated(size_t previousViewerIndex, size_t activeViewerIndex, EditorViewportViewer& activeViewer)
	{
		if (previousViewerIndex != activeViewerIndex && previousViewerIndex != static_cast<size_t>(-1))
		{
			RemovePendingComponents();
			RemovePendingEntities();
			m_ViewportTabs.StoreViewerState(previousViewerIndex, m_Context.SelectedEntity, m_PreviousSelectedEntity);
		}

		if (m_Context.CurrentSceneState != SceneState::Edit)
			return;

		SharedPtr<Scene> scene = activeViewer.GetScene();
		if (!scene)
			return;

		m_Context.IsEditingPrefab = activeViewer.GetType() == EditorViewportViewer::Type::Prefab;
		if (m_Context.IsEditingPrefab)
		{
			auto& prefabViewer = static_cast<PrefabViewportViewer&>(activeViewer);
			m_PrefabEditScene = prefabViewer.GetScene();
			m_EditingPrefab = prefabViewer.PrefabAsset;
			m_EditingPrefabPath = prefabViewer.GetFilePath();
			m_Context.ActivePrefabPath = m_EditingPrefabPath;
			m_Context.PrefabRootEntity = prefabViewer.RootEntity;
		}
		else
		{
			m_EditorScene = scene;
			m_PrefabEditScene = nullptr;
			m_EditingPrefab = nullptr;
			m_EditingPrefabPath.clear();
			m_Context.ActivePrefabPath.clear();
			m_Context.PrefabRootEntity = Entity();
		}

		Application::Instance().GetSceneManager().SetActiveScene(scene);
		if (m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f)
			scene->OnViewportResize(static_cast<uint32_t>(m_ViewportSize.x), static_cast<uint32_t>(m_ViewportSize.y));

		m_Context.SelectedEntity = ResolveEntityInScene(scene, activeViewer.SelectedEntity);
		if (m_Context.SelectedEntity == Constants::Entities::InvalidEntityID)
			m_Context.SelectedEntity = Entity();

		m_PreviousSelectedEntity = ResolveEntityInScene(scene, activeViewer.PreviousSelectedEntity);
		if (m_PreviousSelectedEntity == Constants::Entities::InvalidEntityID)
			m_PreviousSelectedEntity = Entity();

		m_EditorRenderPassSettings.SelectedEntity = m_Context.SelectedEntity != Constants::Entities::InvalidEntityID
			? m_Context.SelectedEntity.GetEntityHandle()
			: Constants::Entities::InvalidEntityID;
	}

	bool EditorLayer::OnViewportViewerCloseRequested(size_t viewerIndex, EditorViewportViewer& viewer, bool saveBeforeClose)
	{
		if (m_Context.CurrentSceneState != SceneState::Edit)
		{
			auto evt = UINotificationEvent("Stop Play mode before closing a scene tab.", UINotificationEvent::Severity::Warning);
			m_Context.EventCallback(evt);
			return false;
		}

		if (viewer.GetType() == EditorViewportViewer::Type::Prefab && saveBeforeClose)
		{
			ActivateViewer(viewerIndex);

			if (m_SavePrefabsWithoutPrompt)
				return SaveOpenPrefab();

			m_PendingPrefabCloseViewerIndex = static_cast<int>(viewerIndex);
			m_ShowClosePrefabPrompt = true;
			return false;
		}

		return true;
	}

	void EditorLayer::ActivateViewer(size_t viewerIndex)
	{
		m_ViewportTabs.ActivateViewer(viewerIndex,
			[this](size_t previousViewerIndex, size_t activeViewerIndex, EditorViewportViewer& activeViewer) { OnViewportViewerActivated(previousViewerIndex, activeViewerIndex, activeViewer); });
	}

	bool EditorLayer::CloseViewer(size_t viewerIndex, bool saveBeforeClose)
	{
		bool closed = m_ViewportTabs.CloseViewer(viewerIndex, saveBeforeClose,
			[this](size_t closeViewerIndex, EditorViewportViewer& viewer, bool save) { return OnViewportViewerCloseRequested(closeViewerIndex, viewer, save); },
			[this](size_t previousViewerIndex, size_t activeViewerIndex, EditorViewportViewer& activeViewer) { OnViewportViewerActivated(previousViewerIndex, activeViewerIndex, activeViewer); });

		if (closed && m_ViewportTabs.Empty())
		{
			m_Context.IsEditingPrefab = false;
			m_Context.ActivePrefabPath.clear();
			m_Context.PrefabRootEntity = Entity();
			m_PrefabEditScene = nullptr;
			m_EditingPrefab = nullptr;
			m_EditingPrefabPath.clear();
			m_EditorScene = nullptr;
			Application::Instance().GetSceneManager().SetActiveScene(nullptr);
			ClearEntitySelectionState();
		}

		return closed;
	}

	bool EditorLayer::CloseAllViewers(bool savePrefabs)
	{
		bool previousSavePrefabsWithoutPrompt = m_SavePrefabsWithoutPrompt;
		m_SavePrefabsWithoutPrompt = savePrefabs;

		bool closed = m_ViewportTabs.CloseAllViewers(savePrefabs,
			[this](size_t closeViewerIndex, EditorViewportViewer& viewer, bool save) { return OnViewportViewerCloseRequested(closeViewerIndex, viewer, save); },
			[this](size_t previousViewerIndex, size_t activeViewerIndex, EditorViewportViewer& activeViewer) { OnViewportViewerActivated(previousViewerIndex, activeViewerIndex, activeViewer); });

		m_SavePrefabsWithoutPrompt = previousSavePrefabsWithoutPrompt;

		if (closed && m_ViewportTabs.Empty())
		{
			m_Context.IsEditingPrefab = false;
			m_Context.ActivePrefabPath.clear();
			m_Context.PrefabRootEntity = Entity();
			m_PrefabEditScene = nullptr;
			m_EditingPrefab = nullptr;
			m_EditingPrefabPath.clear();
			m_EditorScene = nullptr;
			Application::Instance().GetSceneManager().SetActiveScene(nullptr);
			ClearEntitySelectionState();
		}

		return closed;
	}

	SharedPtr<Scene> EditorLayer::LoadSceneForViewer(const std::string& scenePath)
	{
		if (scenePath.empty())
			return nullptr;

		auto& assetManager = Application::Instance().GetAssetManager();
		SharedPtr<Scene> scene = assetManager.Load<Scene>(scenePath, false);
		if (!scene)
			return nullptr;

		SceneSerializer serializer(scene);
		if (!serializer.Deserialize(scenePath))
		{
			auto evt = UINotificationEvent(std::format("Failed to open scene: {}", std::filesystem::path(scenePath).filename().string()), UINotificationEvent::Severity::Error);
			m_Context.EventCallback(evt);
			return nullptr;
		}

		return scene;
	}

	void EditorLayer::OpenSceneViewer(const std::string& scenePath)
	{
		if (scenePath.empty())
			return;

		int existingViewerIndex = m_ViewportTabs.FindViewer(EditorViewportViewer::Type::Scene, scenePath);
		if (existingViewerIndex >= 0)
		{
			ActivateViewer(static_cast<size_t>(existingViewerIndex));
			return;
		}

		SharedPtr<Scene> scene = LoadSceneForViewer(scenePath);
		if (!scene)
			return;

		std::string sceneFilePath = scene->GetFilePath().empty() ? EditorViewportTabs::NormalizedPath(scenePath).string() : scene->GetFilePath();
		std::string title = EditorViewportTabs::TitleFromPath(sceneFilePath, scene->GetName());
		size_t viewerIndex = m_ViewportTabs.AddSceneViewer(scene, sceneFilePath, title);
		ActivateViewer(viewerIndex);

		auto evt = UINotificationEvent(std::format("Opened scene: {}", std::filesystem::path(sceneFilePath).filename().string()));
		m_Context.EventCallback(evt);
	}

	void EditorLayer::OpenPrefabViewer(const std::string& prefabPath)
	{
		if (prefabPath.empty())
			return;

		int existingViewerIndex = m_ViewportTabs.FindViewer(EditorViewportViewer::Type::Prefab, prefabPath);
		if (existingViewerIndex >= 0)
		{
			ActivateViewer(static_cast<size_t>(existingViewerIndex));
			return;
		}

		auto& assetManager = Application::Instance().GetAssetManager();
		auto prefab = assetManager.Load<Prefab>(prefabPath, false);
		if (!prefab)
		{
			auto evt = UINotificationEvent(std::format("Failed to open prefab: {}", std::filesystem::path(prefabPath).filename().string()), UINotificationEvent::Severity::Error);
			m_Context.EventCallback(evt);
			return;
		}

		SharedPtr<Scene> prefabScene = SharedPtr<Scene>::Create(std::format("Prefab: {}", prefab->GetName()), prefab->GetFilePath());
		SceneSerializer serializer(prefabScene);
		Entity prefabRoot = serializer.DeserializePrefab(prefab, true);
		if (prefabRoot == Constants::Entities::InvalidEntityID)
		{
			auto evt = UINotificationEvent(std::format("Failed to deserialize prefab: {}", prefab->GetName()), UINotificationEvent::Severity::Error);
			m_Context.EventCallback(evt);
			return;
		}

		auto& prefabComponent = prefabRoot.ContainsComponent<PrefabComponent>()
			? prefabRoot.GetComponent<PrefabComponent>()
			: prefabRoot.AttachComponent<PrefabComponent>();
		prefabComponent.PrefabHandle = prefab->GetUUID();

		for (Entity entity : prefabScene->GetAllEntities())
		{
			if (entity == prefabRoot)
				continue;

			auto& relationship = entity.GetComponent<RelationshipComponent>();
			if (relationship.ParentHandle == Constants::InvalidUUID)
				prefabScene->SetEntityParent(entity.GetUUID(), prefabRoot);
		}

		std::string prefabFilePath = prefab->GetFilePath().empty() ? EditorViewportTabs::NormalizedPath(prefabPath).string() : prefab->GetFilePath();
		std::string title = std::format("{} [Prefab]", EditorViewportTabs::TitleFromPath(prefabFilePath, prefab->GetName()));
		size_t viewerIndex = m_ViewportTabs.AddPrefabViewer(prefabScene, prefab, prefabRoot, prefabFilePath, title);
		m_ViewportTabs.StoreViewerState(viewerIndex, prefabRoot, Entity());
		ActivateViewer(viewerIndex);

		auto evt = UINotificationEvent(std::format("Opened prefab: {}", prefab->GetName()));
		m_Context.EventCallback(evt);
	}

	void EditorLayer::OpenAnimationViewer(const std::string& animationStatePath)
	{
		if (animationStatePath.empty())
			return;

		int existingViewerIndex = m_ViewportTabs.FindViewer(EditorViewportViewer::Type::Animation, animationStatePath);
		if (existingViewerIndex >= 0)
		{
			ActivateViewer(static_cast<size_t>(existingViewerIndex));
			return;
		}

		auto& assetManager = Application::Instance().GetAssetManager();
		auto animationController = assetManager.Load<AnimationController>(animationStatePath, false);
		if (!animationController)
			return;

		std::string controllerFilePath = animationController->GetFilePath().empty() ? EditorViewportTabs::NormalizedPath(animationStatePath).string() : animationController->GetFilePath();
		std::string title = EditorViewportTabs::TitleFromPath(animationStatePath, animationController->GetName());
		size_t viewerIndex = m_ViewportTabs.AddAnimationViewer(m_Context.ActiveScene(), animationController, controllerFilePath, title);
		ActivateViewer(viewerIndex);

		auto evt = UINotificationEvent(std::format("Opened Animation Controller: {}", std::filesystem::path(animationStatePath).filename().string()));
		m_Context.EventCallback(evt);
	}


	void EditorLayer::OpenSkeletonMaskViewer(const std::string& skeletonMaskPath)
	{
		if (skeletonMaskPath.empty())
			return;

		int existingViewerIndex = m_ViewportTabs.FindViewer(EditorViewportViewer::Type::SkeletonMask, skeletonMaskPath);
		if (existingViewerIndex >= 0)
		{
			ActivateViewer(static_cast<size_t>(existingViewerIndex));
			return;
		}

		auto& assetManager = Application::Instance().GetAssetManager();
		auto skeletonMask = assetManager.Load<SkeletonMask>(skeletonMaskPath, false);
		if (!skeletonMask)
			return;

		std::string skeletonFilePath = skeletonMask->GetFilePath().empty() ? EditorViewportTabs::NormalizedPath(skeletonMaskPath).string() : skeletonMask->GetFilePath();
		std::string title = EditorViewportTabs::TitleFromPath(skeletonMaskPath, skeletonMask->GetName());
		size_t viewerIndex = m_ViewportTabs.AddSkeletonMaskViewer(m_Context.ActiveScene(), skeletonMask, skeletonFilePath, title);
		ActivateViewer(viewerIndex);

		auto evt = UINotificationEvent(std::format("Opened Skeleton Mask: {}", std::filesystem::path(skeletonMaskPath).filename().string()));
		m_Context.EventCallback(evt);
	}

	void EditorLayer::SetNewScene(SharedPtr<Scene> newScene)
	{
		CloseAllViewers(false);
		size_t viewerIndex = m_ViewportTabs.AddSceneViewer(newScene, newScene->GetFilePath(), EditorViewportTabs::TitleFromPath(newScene->GetFilePath(), newScene->GetName()));
		ActivateViewer(viewerIndex);
		ClearEntitySelectionState();
	}

	void EditorLayer::SetupImGuiTheme()
	{
		ImGuiIO& io = ImGui::GetIO();

		// Keep the docking layout per-user: an installed build cannot write beside its own executable.
		// ImGui stores the pointer without copying, so the string has to outlive the context. Settings
		// are only read on the first NewFrame, which is still ahead of us here.
		static std::string iniPath = (Paths::UserDataDir() / "imgui.ini").string();
		io.IniFilename = iniPath.c_str();

		// Seed a first run with the layout we ship rather than ImGui's undocked default.
		const std::filesystem::path defaultLayout = Paths::EditorAssets() / "imgui.ini";
		if (!std::filesystem::exists(iniPath) && std::filesystem::exists(defaultLayout))
		{
			std::error_code error;
			std::filesystem::copy_file(defaultLayout, iniPath, error);
			if (error)
				EB_WARN("Could not seed the default editor layout: {}", error.message());
		}

		io.Fonts->AddFontFromFileTTF((Paths::EditorAssets() / "fonts/Roboto-Regular.ttf").string().c_str(), 16.0f);

		ImGuiStyle& style = ImGui::GetStyle();
		ImVec4* colors = style.Colors;

		// --- Sizing & Spacing (Slightly denser for higher information density) ---
		style.WindowPadding = ImVec2(10.0f, 9.0f);
		style.FramePadding = ImVec2(6.0f, 4.0f);
		style.ItemSpacing = ImVec2(8.0f, 6.0f);
		style.ItemInnerSpacing = ImVec2(5.0f, 4.0f);
		style.IndentSpacing = 16.0f;
		style.ScrollbarSize = 12.0f;
		style.GrabMinSize = 10.0f;

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

		// Defaults for these two are light grey, which washes the editor out whenever a modal
		// is open or the dockspace has no docked windows
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.02f, 0.02f, 0.03f, 0.55f);
		colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);

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
