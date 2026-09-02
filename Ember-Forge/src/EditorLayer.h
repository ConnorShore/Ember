#pragma once

#include "Ember/Tools/EditorCamera.h"
#include "Panels/Panel.h"
#include "EditorContext.h"
#include "ProjectSettingsDialog.h"
#include "WelcomeDialog.h"
#include "Utils/ViewportGizmoController.h"
#include "Undo/SelectionEditTracker.h"
#include "Viewers/EditorViewportTabs.h"

#include <Ember/Event/KeyEvent.h>
#include <Ember/Event/MouseEvent.h>
#include <Ember/Render/Framebuffer.h>
#include <Ember/Render/RenderPassSettings.h>

#include <ImGuizmo.h>
#include <vector>
#include <concepts>

namespace Ember {

	class EditorLayer : public Layer
	{
	public:
		EditorLayer();
		virtual ~EditorLayer();

		void OnAttach() override;
		void OnDetach() override;
		void OnEvent(Event& event) override;
		void OnUpdate(TimeStep delta) override;
		void OnImGuiRender(TimeStep delta) override;

		// --- Public Access for Viewers ---
		EditorContext& GetContext() { return m_Context; }
		EditorCamera& GetCamera() { return m_Camera; }
		SharedPtr<Framebuffer> GetOutputFramebuffer() { return m_OutputFramebuffer; }
		SharedPtr<Framebuffer> GetCameraPreviewFramebuffer() { return m_CameraPreviewFramebuffer; }
		ViewportGizmoController& GetViewportGizmos() { return m_ViewportGizmos; }
		int GetGizmoType() const { return m_GizmoType; }

		bool IsViewportHovered() const { return m_ViewportHovered; }
		void SetViewportHovered(bool hovered) { m_ViewportHovered = hovered; }
		bool IsViewportFocused() const { return m_ViewportFocused; }
		void SetViewportFocused(bool focused) { m_ViewportFocused = focused; }

		Vector2f GetViewportSize() const { return m_ViewportSize; }
		void SetViewportSize(const Vector2f& size) { m_ViewportSize = size; }
		void SetViewportBounds(const Vector2f& min, const Vector2f& max) { m_ViewportBounds[0] = min; m_ViewportBounds[1] = max; }
		Vector2f* GetViewportBounds() { return m_ViewportBounds; }
		void SetCameraPreviewViewportSize(const Vector2f& size) { m_CameraPreviewViewportSize = size; }

		// Where a newly created entity should go: the surface under the cursor when there is one, so
		// nothing silently lands at the world origin underneath the level.
		Vector3f GetSpawnPosition();

		void CreateEntityFromModel(const std::string& modelFilePath);
		void CreateEntityFromPrefab(const std::string& prefabFilePath);
		void OpenPrefab(const std::string& prefabPath = "");
		void OpenScene(const std::string& scenePath = "");

	private:
		void LoadDefaultAssets();

		void OnRuntimeStart();
		void OnRuntimeStop();
		void StopRuntimeAfterError(const std::string& message);
		void StopRuntimeAfterError(const char* message);

		void RenderMenuBar();
		void RenderSceneViewport();
		void RenderNewProjectPopup();
		void RenderNewScenePopup();
		void RenderClosePrefabPrompt();

		bool OnKeyPressed(KeyPressedEvent& e);
		bool OnMouseClick(MousePressedEvent& e);
		bool OnQuitRequested(QuitRequestedEvent& e);
		void SyncEntitySelectionState();
		void SyncSelectionOutlines();

		void Undo();
		void Redo();

		uint32_t RefreshPrefabInstancesInOpenScenes(const SharedPtr<Prefab>& prefab);
		uint32_t CountPrefabInstancesInOpenScenes(const SharedPtr<Prefab>& prefab);
		void RenderPrefabRefreshPrompt();

		void DrawToolbar();

		// One dropdown for every editor toggle, so the toolbar does not grow a control per setting.
		void DrawGizmoSettingsPopup();
		bool DrawScreenSpaceUIToggles();
		void DrawDebugDrawToggles();

		// Orbits the editor camera to frame the selection and everything under it.
		void FocusSelection();

		// Viewport-local pixel under the cursor, Y flipped for OpenGL; false when outside the image.
		bool TryGetViewportPixel(int& outX, int& outY) const;

		void RenderStatsOverlay(TimeStep delta);
		float CalculateFPS(TimeStep delta);
		void ClearEntitySelectionState();

		void CreateEntity();
		void RemoveEntity(Entity entity);
		void RemovePendingEntities();
		void RemovePendingComponents();

		void SyncEditorIconComponents(Scene* scene);

		void OutlineEntity(Entity entity);

		// Requests, not completions - NewProject only queues the popup, so neither can report the resulting project
		void NewProject();
		void OpenProject(const std::string& projectFilePath = "");

		// The one point every route converges on once a project is actually active
		void OnProjectOpened(const std::filesystem::path& projectFilePath);
		void NewScene();
		void SaveScene(bool saveAs = false);
		void SaveProject();
		void SerializeScene(const SharedPtr<Scene>& scene, const std::string& filePath);
		bool SaveOpenPrefab();
		void OpenAnimationState(const std::string& path);
		void OpenSkeletonMask(const std::string& path);
		void HandleSceneOpenRequest();
		void HandlePrefabOpenRequest();
		void HandleAnimationOpenRequest();
		void HandleSkeletonMaskOpenRequest();

		EditorViewportViewer* GetActiveViewer();
		const EditorViewportViewer* GetActiveViewer() const;
		void OnViewportViewerActivated(size_t previousViewerIndex, size_t activeViewerIndex, EditorViewportViewer& activeViewer);
		bool OnViewportViewerCloseRequested(size_t viewerIndex, EditorViewportViewer& viewer, bool saveBeforeClose);
		void ActivateViewer(size_t viewerIndex);
		bool CloseViewer(size_t viewerIndex, bool saveBeforeClose);
		bool CloseAllViewers(bool savePrefabs);
		SharedPtr<Scene> LoadSceneForViewer(const std::string& scenePath);
		void OpenSceneViewer(const std::string& scenePath);
		void OpenPrefabViewer(const std::string& prefabPath);
		void OpenAnimationViewer(const std::string& animationStatePath);
		void OpenSkeletonMaskViewer(const std::string& skeletonMaskPath);

		void SetNewScene(SharedPtr<Scene> newScene);

		void SetupImGuiTheme();

		template<std::derived_from<Panel> T>
		SharedPtr<T> GetPanel()
		{
			for (auto panel : m_Panels)
			{
				if (DynamicPointerCast<T>(panel) != nullptr)
				{
					return DynamicPointerCast<T>(panel);
				}
			}

			return nullptr;
		}

	private:
		template<typename T>
		inline void RemoveComponentFromEntity(Entity entity)
		{
			if (entity == Constants::Entities::InvalidEntityID)
				return;

			ComponentType type = entity.GetComponentType<T>();
			if (type == Constants::Entities::InvalidComponentID)
				return;

			m_Context.PendingComponentRemovals[entity].push_back(type);
		}

		template<typename T>
		inline bool CancelComponentRemoval(Entity entity)
		{
			if (entity == Constants::Entities::InvalidEntityID)
				return false;

			ComponentType type = entity.GetComponentType<T>();
			if (type == Constants::Entities::InvalidComponentID)
				return false;

			auto& pendingRemovals = m_Context.PendingComponentRemovals[entity];
			size_t numErased = std::erase(pendingRemovals, type);
			if (pendingRemovals.empty())
			{
				m_Context.PendingComponentRemovals.erase(entity);
			}

			return numErased > 0;
		}

	private:
		const Entity m_InvalidEntity = Entity(Constants::Entities::InvalidEntityID, nullptr);

	private:
		EditorContext m_Context;
		EditorPreferences m_Preferences;
		SharedPtr<Scene> m_EditorScene;
		SharedPtr<Scene> m_PrefabEditScene;
		SharedPtr<Prefab> m_EditingPrefab;
		std::string m_EditingPrefabPath;
		EditorViewportTabs m_ViewportTabs;

		EditorCamera m_Camera;
		SharedPtr<Framebuffer> m_OutputFramebuffer;
		SharedPtr<Framebuffer> m_CameraPreviewFramebuffer;

		Vector2f m_ViewportBounds[2];
		Vector2f m_ViewportSize;
		Vector2f m_CameraPreviewViewportSize = { 320.0f, 180.0f };

		std::vector<SharedPtr<Panel>> m_Panels;

		bool m_ViewportHovered = false;
		bool m_ViewportFocused = false;

		bool m_ShowStatsWindow = false;
		bool m_SkipSceneUiThisFrame = false;
		bool m_ShowClosePrefabPrompt = false;
		bool m_SavePrefabsWithoutPrompt = false;
		int m_PendingPrefabCloseViewerIndex = -1;

		Entity m_PreviousSelectedEntity = m_InvalidEntity;

		// Which entities currently carry an OutlineComponent, and the selection that produced them.
		std::unordered_set<Entity> m_PreviouslyOutlined;
		std::vector<Entity> m_LastOutlinedSelection;

		SelectionEditTracker m_EditTracker;

		// Set when a prefab save finds placed instances; the prompt asks before overwriting them.
		SharedPtr<Prefab> m_PendingRefreshPrefab;
		uint32_t m_PendingRefreshInstanceCount = 0;
		OutlineComponent m_OutlineEntitySelectedComp = { Vector3f(0.89f, 0.25f, 0.07f), 2.0f };

		int m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
		ViewportGizmoController m_ViewportGizmos;

		Entity m_EntityToDelete;

		bool m_ShowNewScenePopup = false;
		std::string m_NewSceneName = "";

		bool m_ShowProjectSettingsPopup = false;
		ProjectSettingsDialog m_ProjectSettingsDialog;
		WelcomeDialog m_WelcomeDialog;

		RenderPassSettings m_EditorRenderPassSettings;

		struct NewProjectSettings
		{
			bool ShowProjectSettingsPopup = false;
			std::string ProjectName;
			std::string ProjectDirectory;
		} m_NewProjectSettings;

		struct ToolbarProps
		{
			uint32_t PlayButtonTextureID;
			uint32_t PauseButtonTextureID;
			uint32_t StopButtonTextureID;
		} m_ToolbarProps;
	};
}