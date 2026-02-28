#include "ebpch.h"
#include "Application.h"
#include "Core.h"

#include <GLFW/glfw3.h>

namespace Ember {

	Application::Application()
	{
		m_Window = ScopedPtr<Window>(Window::Create());
		m_Window->SetEventCallback(EB_EVENT_FUNCTION(OnEvent(e)));
		EB_CORE_INFO("Application created!");
	}

	Application::~Application()
	{
		EB_CORE_INFO("Application destroyed!");
	}

	void Application::OnEvent(Event& event)
	{
		EB_CREATE_DISPATCHER(event)
		EB_DISPATCH_EVENT(WindowCloseEvent, OnWindowClose);
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

	bool Application::OnWindowClose(WindowCloseEvent e)
	{
		m_Running = false;
		return true;
	}

}