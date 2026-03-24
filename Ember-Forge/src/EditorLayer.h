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
		bool OnKeyPressed(KeyPressedEvent& e);
		bool OnMouseClick(MousePressedEvent& e);
		void SyncEntitySelectionState();
		void RenderTransformGizmos();

		void CreateEntity();
		void RemoveEntity(Entity entity);
		void RemovePendingEntities();

		void NewScene();
		void OpenScene();
		void SaveScene(bool saveAs = false);

	private:
		const Entity m_InvalidEntity = Entity(Constants::Entities::InvalidEntityID, nullptr);

	private:
		EditorContext m_Context;
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
	};

}