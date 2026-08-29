#include "ebpch.h"
#include "Application.h"
#include "Core.h"
#include "Paths.h"
#include "Random.h"
#include "Ember/Input/Input.h"
#include "Ember/Render/RenderAction.h"
#include "Ember/Asset/Serializers/AssetRegistrySerializer.h"

#include "Ember/ECS/System/ScriptSystem.h"
#include "Ember/ECS/System/PhysicsSystem.h"
#include "Ember/ECS/System/Rendersystem.h"
#include "Ember/ECS/System/TransformSystem.h"
#include "Ember/ECS/System/AnimationSystem.h"
#include "Ember/ECS/System/VisibilitySystem.h"
#include "Ember/ECS/System/BoneSocketSystem.h"
#include "Ember/ECS/System/CharacterControllerSystem.h"
#include "Ember/ECS/System/LifecycleSystem.h"
#include "Ember/ECS/System/ParticleSystem.h"
#include "Ember/ECS/System/AudioSystem.h"
#include "Ember/ECS/System/AISystem.h"
#include "Ember/ECS/System/UILayoutSystem.h"
#include "Ember/ECS/System/UIInputSystem.h"

#include "Ember/Script/ScriptEngine.h"

#include <GLFW/glfw3.h>

namespace Ember {

	Application* Application::s_Instance = nullptr;

	Application::Application(const ApplicationSpecification& applicationSpes)
		: m_Specification(applicationSpes)
	{
		EB_CORE_ASSERT(s_Instance == nullptr, "Application instance is alredy created!");
		EB_PROFILE_FUNCTION();

		s_Instance = this;

		m_Window = Window::Create(applicationSpes.WindowSpecification);
		m_Window->SetEventCallback(EB_EVENT_FUNCTION(OnEvent(e)));

		m_ImGuiLayer.OnAttach();

		m_AssetManager.SetEngineAssetDirectory(applicationSpes.EngineAssetDir);
		m_AssetManager.SetProjectAssetDirectory(applicationSpes.ProjectAssetDir);
		m_AssetManager.LoadDefaults();

		// m_SaveGameManager and m_InputActions are members by value, so both already exist here -
		// which matters, because binding the script API publishes `GameData` as a pointer to the
		// save-game manager. See the note on their declarations in Application.h.
		Random::Init();
		ScriptEngine::Init();

		m_SystemManager.RegisterSystem(SharedPtr<ScriptSystem>::Create());
		m_SystemManager.RegisterSystem(SharedPtr<VisibilitySystem>::Create());
		m_SystemManager.RegisterSystem(SharedPtr<AnimationSystem>::Create());
		m_SystemManager.RegisterSystem(SharedPtr<PhysicsSystem>::Create());
		m_SystemManager.RegisterSystem(SharedPtr<TransformSystem>::Create());
		m_SystemManager.RegisterSystem(SharedPtr<BoneSocketSystem>::Create());
		m_SystemManager.RegisterSystem(SharedPtr<ParticleSystem>::Create(Constants::Renderer::MaxParticles));
		m_SystemManager.RegisterSystem(SharedPtr<RenderSystem>::Create());
		m_SystemManager.RegisterSystem(SharedPtr<CharacterControllerSystem>::Create());
		m_SystemManager.RegisterSystem(SharedPtr<LifecycleSystem>::Create());
		m_SystemManager.RegisterSystem(SharedPtr<AudioSystem>::Create());
		m_SystemManager.RegisterSystem(SharedPtr<AISystem>::Create());
		m_SystemManager.RegisterSystem(SharedPtr<UILayoutSystem>::Create());
		m_SystemManager.RegisterSystem(SharedPtr<UIInputSystem>::Create());

		EB_CORE_INFO("Application created!");
	}

	Application::~Application()
	{
		for (auto& layer : m_LayerStack)
			layer->OnDetach();

		m_ImGuiLayer.OnDetach();

		// Written per-user rather than into the engine asset directory, which an installed build has
		// no permission to modify.
		AssetRegistrySerializer serializer(&m_AssetManager);
		serializer.Serialize((Paths::UserDataDir() / "assets.eba").string());

		//ScriptEngine::Shutdown();

		EB_CORE_INFO("Application destroyed!");
	}

	void Application::PushLayer(ScopedPtr<Layer> layer)
	{
		layer->SetAssetManagerHandle(&m_AssetManager);
		m_LayerStack.PushLayer(std::move(layer));
	}

	void Application::PushCanvasLayer(ScopedPtr<Layer> canvas)
	{
		canvas->SetAssetManagerHandle(&m_AssetManager);
		m_LayerStack.PushCanvasLayer(std::move(canvas));
	}

	void Application::OnAttach()
	{
		EB_CORE_INFO("Application attached!");
	}

	void Application::OnDetach()
	{
		EB_CORE_INFO("Application Detached!");
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
		EB_DISPATCH_EVENT(MouseScrolledEvent, OnMouseScrolled);
		EB_DISPATCH_EVENT(MouseMovedEvent, OnMouseMoved);

		for (auto& layer : m_LayerStack)
			layer->OnEvent(event);
	}

	void Application::Run()
	{
		EB_CORE_INFO("Application running!");

		TimeStamp lastTime = Timer::Now();
		while (m_Running) {
			EB_PROFILE_SCOPE("Frame");

			TimeStamp currentTime = Timer::Now();
			TimeStep delta = currentTime - lastTime;
			lastTime = currentTime;

			Input::BeginFrame();
			m_InputActions.Evaluate();

			{
				EB_PROFILE_SCOPE("LayerStack::OnUpdate");
				for (auto& layer : m_LayerStack)
					layer->OnUpdate(delta);
			}

			{
				EB_PROFILE_SCOPE("ImGuiLayer::Frame");
				m_ImGuiLayer.BeginFrame();

				for (auto& layer : m_LayerStack)
					layer->OnImGuiRender(delta);

				m_ImGuiLayer.EndFrame();
			}

			Input::ResetMouseDelta();

			{
				// Includes glfwPollEvents (OS/input) and SwapBuffers (blocks on vsync if enabled) —
				// see Windows::Window::OnUpdate for the finer-grained split of the two.
				EB_PROFILE_SCOPE("Window::OnUpdate");
				m_Window->OnUpdate();
			}

			{
				EB_PROFILE_SCOPE("SceneManager::ExecuteSceneSwap");
				m_SceneManager.ExecuteSceneSwap();
			}
		}

		EB_CORE_INFO("Application stopped running!");
	}

	void Application::Close()
	{
		m_Running = false;
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		Close();
		return true;
	}

	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		EB_CORE_TRACE("Window resized to {}x{}", e.GetWidth(), e.GetHeight());
		RenderAction::SetViewport(0, 0, e.GetWidth(), e.GetHeight());
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
		Input::SetMouseControlState(e.GetMouseControl(), true);
		return false;
	}

	bool Application::OnMouseReleased(MouseReleasedEvent& e)
	{
		Input::SetMouseControlState(e.GetMouseControl(), false);
		return false;
	}

	bool Application::OnMouseScrolled(MouseScrolledEvent& e)
	{
		Input::SetMouseScrollOffset(e.GetScrollOffset());
		return false;
	}

	bool Application::OnMouseMoved(MouseMovedEvent& e)
	{
		Input::UpdateMousePosition(e.GetMousePosition());
		return false;
	}

}