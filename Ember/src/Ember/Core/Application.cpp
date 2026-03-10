 #include "ebpch.h"
#include "Application.h"
#include "Core.h"
#include "Ember/Input/Input.h"

#include <GLFW/glfw3.h>

namespace Ember {

	Application* Application::s_Instance = nullptr;

	Application::Application()
	{
		EB_CORE_ASSERT(s_Instance == nullptr, "Application instance is alredy created!");

		s_Instance = this;

		m_Window = Window::Create();
		m_Window->SetEventCallback(EB_EVENT_FUNCTION(OnEvent(e)));

		m_ImGuiLayer = ScopedPtr<ImGuiLayer>::Create();
		m_ImGuiLayer->OnAttach();

		m_ShaderLibrary = ScopedPtr<ShaderLibrary>::Create();
		m_TextureLibrary = ScopedPtr<TextureLibrary>::Create();

		EB_CORE_INFO("Application created!");
	}

	Application::~Application()
	{
		m_ImGuiLayer->OnDetatch();

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

	void Application::RegisterShader(const std::string& filePath)
	{
		auto shader = m_ShaderLibrary->Register(filePath);
		EB_CORE_INFO("Successfully registered shader {}", shader->GetName());
	}

	SharedPtr<Shader> Application::GetShader(const std::string& name)
	{
		return m_ShaderLibrary->Get(name);
	}

	void Application::RegisterTexture(const std::string& filePath)
	{
		auto texture = m_TextureLibrary->Register(filePath);
		EB_CORE_INFO("Successfully registered texture {}", texture->GetName());
	}

	SharedPtr<Texture> Application::GetTexture(const std::string& name)
	{
		return m_TextureLibrary->Get(name);
	}

	void Application::OnAttach()
	{
		EB_CORE_INFO("Application attached!");
	}

	void Application::OnDetatch()
	{
		EB_CORE_INFO("Application detatched!");
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

		TimeStamp lastTime = Timer::Now();
		while (m_Running) {
			TimeStamp currentTime = Timer::Now();
			TimeStep delta = currentTime - lastTime;
			lastTime = currentTime;

			for (auto& layer : m_LayerStack)
				layer->OnUpdate(delta);

			m_ImGuiLayer->BeginFrame();

			for (auto& layer : m_LayerStack)
				layer->OnImGuiRender(delta);

			m_ImGuiLayer->EndFrame();

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