#pragma once

#include <Ember.h>

namespace Ember {

	class RuntimeLayer : public Layer
	{
	public:
		RuntimeLayer() : Layer("Runtime Layer") {}
		virtual ~RuntimeLayer() = default;

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate(TimeStep delta) override;
		void OnImGuiRender(TimeStep delta) override;
		void OnEvent(Event& event) override;

	private:
		bool OnWindowResize(WindowResizeEvent& e);
		bool OnKeyPressed(KeyPressedEvent& e);
		float CalculateFPS(TimeStep delta);

		bool m_ShowFPSOverlay = false;
	};

}