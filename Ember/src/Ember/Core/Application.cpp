#include "ebpch.h"
#include "Application.h"
#include "Core.h"

#include <GLFW/glfw3.h>

namespace Ember {

	Application::Application()
	{
		m_Window = ScopedPtr<Window>(Window::Create());
		EB_CORE_INFO("Application created!");
	}

	Application::~Application()
	{
		EB_CORE_INFO("Application destroyed!");
	}

	void Application::Run()
	{
		EB_CORE_INFO("Application running!");
		
		while (m_Running)
		{
			m_Window->OnUpdate();
		}

		EB_CORE_INFO("Application stopped running!");
	}
}