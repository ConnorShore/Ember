#pragma once

#include "Ember/Tools/EditorCamera.h"
#include "Panels/Panel.h"

#include <Ember.h>
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
		void SetupDirectionalLights();
		bool OnMouseClick(MousePressedEvent& e);

	private:
		const Entity m_InvalidEntity = Entity(Constants::Entities::InvalidEntityID, nullptr);

	private:
		EditorCamera m_Camera;

		SharedPtr<Scene> m_ActiveScene;
		SharedPtr<Framebuffer> m_OutputFramebuffer;

		Vector2f m_ViewportBounds[2];	// Top Left and Bottom Right corners in screen space
		Vector2f m_ViewportSize;
		Vector2f m_SceneViewSize;

		Entity m_SelectedEntity;

		std::vector<SharedPtr<Panel>> m_Panels;

		bool m_ViewportHovered = false;
		bool m_ViewportFocused = false;
	};

}