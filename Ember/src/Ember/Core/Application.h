#pragma once

#include "Window.h"
#include "ScopedPointer.h"

namespace Ember {

	class Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();
		bool IsRunning() const { return m_Running; }

	private:
		ScopedPtr<Window> m_Window;
		bool m_Running = true;
	};

	Application* CreateApplication();
}

