#pragma once

#include "Ember/Tools/EditorCamera.h"
#include "Panels/Panel.h"
#include "EditorContext.h"
#include "ProjectSettingsDialog.h"

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
		void RenderWelcomePopup();

		bool OnKeyPressed(KeyPressedEvent& e);
		bool OnMouseClick(MousePressedEvent& e);
		void SyncEntitySelectionState();
		void RenderTransformGizmos();
		void RenderRectTransformGizmos();

		void DrawToolbar();

		void RenderStatsOverlay(TimeStep delta);
		float CalculateFPS(TimeStep delta);

		void CreateEntity();
		void RemoveEntity(Entity entity);
		void RemovePendingEntities();
		void RemovePendingComponents();

		void CreateEntityFromModel(const std::string& modelFilePath);
		void CreateEntityFromPrefab(const std::string& prefabFilePath);

		void OutlineEntity(Entity entity);

		void NewProject();
		void OpenProject();
		void NewScene();
		void OpenScene(const std::string& scenePath = "");
		void SaveProject(bool saveAs = false);

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
		SharedPtr<Scene> m_EditorScene;

		EditorCamera m_Camera;
		SharedPtr<Framebuffer> m_OutputFramebuffer;
		SharedPtr<Framebuffer> m_CameraPreviewFramebuffer;

		Vector2f m_ViewportBounds[2];	// Top Left and Bottom Right corners in screen space
		Vector2f m_ViewportSize;
		Vector2f m_CameraPreviewViewportSize = { 320.0f, 180.0f };

		std::vector<SharedPtr<Panel>> m_Panels;

		bool m_ViewportHovered = false;
		bool m_ViewportFocused = false;

		bool m_ShowStatsWindow = false;

		bool m_DrawAllHUD = false;

		Entity m_PreviousSelectedEntity = m_InvalidEntity;
		OutlineComponent m_OutlineEntitySelectedComp = { Vector3f(0.89f, 0.25f, 0.07f), 2.0f };

		enum class RectTransformGizmoHandle
		{
			None,
			Move,
			Left,
			Right,
			Top,
			Bottom,
			TopLeft,
			TopRight,
			BottomLeft,
			BottomRight,
			Rotate
		};

		int m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
		int m_GizmoMode = ImGuizmo::WORLD;
		bool m_RectTransformGizmoHovered = false;
		EntityID m_RectTransformGizmoEntity = Constants::Entities::InvalidEntityID;
		RectTransformGizmoHandle m_RectTransformGizmoActiveHandle = RectTransformGizmoHandle::None;
		Vector2f m_RectTransformGizmoLastMousePosition = Vector2f(0.0f);

		Entity m_EntityToDelete;

		bool m_ShowNewScenePopup = false;
		std::string m_NewSceneName = "";
		
		bool m_ShowProjectSettingsPopup = false;
		ProjectSettingsDialog m_ProjectSettingsDialog;

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