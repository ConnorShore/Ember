#pragma once

#include "Window.h"
#include "Ember/Event/Event.h"
#include "Ember/Event/WindowEvent.h"
#include "ScopedPointer.h"

namespace Ember {

	class Application
	{
	public:
		Application();
		virtual ~Application();

		void OnEvent(Event& event);

		void Run();
		bool IsRunning() const { return m_Running; }

	private:
		bool OnWindowClose(WindowCloseEvent e);

	private:
		ScopedPtr<Window> m_Window;
		bool m_Running = true;
	};

	Application* CreateApplication();
}

