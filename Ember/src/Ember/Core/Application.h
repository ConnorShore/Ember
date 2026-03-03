#pragma once

#include "Window.h"
#include "LayerStack.h"
#include "ScopedPointer.h"

#include "Ember/Event/Event.h"
#include "Ember/Event/WindowEvent.h"
#include "Ember/Event/KeyEvent.h"
#include "Ember/Event/MouseEvent.h"
#include "Ember/ImGui/ImGuiLayer.h"

namespace Ember {

	class Application
	{
	public:
		Application();
		virtual ~Application();

		void PushLayer(ScopedPtr<Layer> layer);
		void PushCanvasLayer(ScopedPtr<Layer> canvas);

		void OnEvent(Event& event);

		void Run();
		bool IsRunning() const { return m_Running; }

		inline static Application& Instance() { return *s_Instance; }

		inline const Window& GetWindow() const { return *m_Window; }

	private:
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);
		bool OnKeyPressed(KeyPressedEvent& e);
		bool OnKeyReleased(KeyReleasedEvent& e);
		bool OnKeyRepeat(KeyRepeatEvent& e);
		bool OnMousePressed(MousePressedEvent& e);
		bool OnMouseReleased(MouseReleasedEvent& e);

	private:
		ScopedPtr<Window> m_Window;
		bool m_Running = true;

		LayerStack m_LayerStack;
		ScopedPtr<ImGuiLayer> m_ImGuiLayer;

		static Application* s_Instance;
	};

	Application* CreateApplication();
}

