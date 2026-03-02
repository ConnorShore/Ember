 #include "ebpch.h"
#include "Application.h"
#include "Core.h"
#include "Ember/Input/Input.h"

#include <GLFW/glfw3.h>

namespace Ember {

	Application::Application()
	{
		m_Window = Window::Create();
		m_Window->SetEventCallback(EB_EVENT_FUNCTION(OnEvent(e)));
		EB_CORE_INFO("Application created!");
	}

	Application::~Application()
	{
		EB_CORE_INFO("Application destroyed!");
	}

	void Application::PushLayer(ScopedPtr<Layer> layer)
	{
		layer->OnAttach();
		m_LayerStack.PushLayer(std::move(layer));
	}

	void Application::PushCanvasLayer(ScopedPtr<Layer> canvas)
	{
		canvas->OnAttach();
		m_LayerStack.PushCanvasLayer(std::move(canvas));
	}

	void Application::OnEvent(Event& event)
	{
		EB_CREATE_DISPATCHER(event)
		EB_DISPATCH_EVENT(WindowCloseEvent, OnWindowClose);
		EB_DISPATCH_EVENT(WindowResizeEvent, OnWindowResize);
		EB_DISPATCH_EVENT(KeyPressedEvent, OnKeyPressed);
		EB_DISPATCH_EVENT(KeyReleasedEvent, OnKeyReleased);
		EB_DISPATCH_EVENT(KeyRepeatEvent, OnKeyRepeat);
		EB_DISPATCH_EVENT(MousePressedEvent, OnMousePressed);
		EB_DISPATCH_EVENT(MouseReleasedEvent, OnMouseReleased);
	}

	void Application::Run()
	{
		EB_CORE_INFO("Application running!");
		
		while (m_Running) {
			m_Window->Clear();

			for (auto& layer : m_LayerStack)
				layer->OnUpdate();

			m_Window->OnUpdate();
		}

		EB_CORE_INFO("Application stopped running!");
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}

	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		EB_CORE_TRACE("Window resized to {}x{}", e.GetWidth(), e.GetHeight());
		return true;
	}

	bool Application::OnKeyPressed(KeyPressedEvent& e)
	{
		Input::SetKeyState(e.GetKeyCode(), true);
		return false;
	}

	bool Application::OnKeyReleased(KeyReleasedEvent& e)
	{
		Input::SetKeyState(e.GetKeyCode(), false);
		return false;
	}

	bool Application::OnKeyRepeat(KeyRepeatEvent& e)
	{
		Input::IncrementKeyRepeat(e.GetKeyCode());
		return false;
	}

	bool Application::OnMousePressed(MousePressedEvent& e)
	{
		Input::SetMouseButtonState(e.GetMouseButton(), true);
		return false;
	}

	bool Application::OnMouseReleased(MouseReleasedEvent& e)
	{
		Input::SetMouseButtonState(e.GetMouseButton(), false);
		return false;
	}

}