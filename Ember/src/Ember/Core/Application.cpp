#include "ebpch.h"
#include "Application.h"
#include "Core.h"

namespace Ember {

	Ember::Application* Application::s_Instance = nullptr;

	Application::Application()
	{
		EB_CORE_INFO("Application created!");
		s_Instance = this;
	}

	Application::~Application()
	{
		EB_CORE_INFO("Application destroyed!");
	}

	void Application::Run()
	{
		EB_CORE_INFO("Application running!");
		m_Running = true;

		while (m_Running) {

		}

		EB_CORE_INFO("Application stopped!");
	}
}