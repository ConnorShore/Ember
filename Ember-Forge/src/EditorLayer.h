#pragma once

#include "Ember/Tools/EditorCamera.h"
#include "Panels/Panel.h"
#include "EditorContext.h"

#include <Ember.h>
#include <ImGuizmo.h>
#include <vector>

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
		void OnRuntimeStart();
		void OnRuntimeStop();

		void RenderMenuBar();
		void RenderSceneViewport();
		void RenderNewProjectPopup();

		bool OnKeyPressed(KeyPressedEvent& e);
		bool OnMouseClick(MousePressedEvent& e);
		void SyncEntitySelectionState();
		void RenderTransformGizmos();

		void DrawToolbar(float fps);

		float CalculateFPS(TimeStep delta);

		void CreateEntity();
		void RemoveEntity(Entity entity);
		void RemovePendingEntities();
		void RemovePendingComponents();

		void CreateEntityFromModel(const std::string& modelFilePath);

		void OutlineEntity(Entity entity);

		void NewProject();
		void OpenProject();
		void NewScene();
		void OpenScene(const std::string& scenePath = "");
		void SaveScene(bool saveAs = false);

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

		enum class SceneState
		{
			Edit = 0,
			Play = 1,
			Pause = 2
		} m_SceneState = SceneState::Edit;

		EditorCamera m_Camera;
		SharedPtr<Framebuffer> m_OutputFramebuffer;

		Vector2f m_ViewportBounds[2];	// Top Left and Bottom Right corners in screen space
		Vector2f m_ViewportSize;
		Vector2f m_SceneViewSize;

		std::vector<SharedPtr<Panel>> m_Panels;

		bool m_ViewportHovered = false;
		bool m_ViewportFocused = false;

		Entity m_PreviousSelectedEntity = m_InvalidEntity;
		OutlineComponent m_OutlineEntitySelectedComp = { Vector3f(0.89f, 0.25f, 0.07f), 2.0f };

		int m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
		int m_GizmoMode = ImGuizmo::WORLD;

		Entity m_EntityToDelete;

		struct NewProjectSettings
		{
			bool ShowProjectSettingsPopup = false;
			std::string ProjectName;
			std::string ProjectDirectory;
		} m_NewProjectSettings;
	};

}