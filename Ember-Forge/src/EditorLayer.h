#pragma once

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

	private:
		SharedPtr<Scene> m_ActiveScene;
		SharedPtr<Framebuffer> m_OutputFramebuffer;
		Vector2f m_ViewportSize;

		Entity m_SelectedEntity;

		Entity m_CameraEntity;	// Will be an EditorCamera

		std::vector<SharedPtr<Panel>> m_Panels;
	};

}