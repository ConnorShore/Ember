#pragma once

#include "Window.h"
#include "LayerStack.h"
#include "ScopedPointer.h"

#include "Ember/Event/Event.h"
#include "Ember/Event/WindowEvent.h"
#include "Ember/Event/KeyEvent.h"
#include "Ember/Event/MouseEvent.h"
#include "Ember/ImGui/ImGuiLayer.h"

#include "Ember/ECS/System/SystemManager.h"

#include "Ember/Asset/AssetManager.h"
#include "Ember/Asset/Asset.h"

namespace Ember {

	class Application
	{
	public:
		Application(const std::string& name = "Ember App", const WindowConfig& config = {});
		virtual ~Application();

		void PushLayer(ScopedPtr<Layer> layer);
		void PushCanvasLayer(ScopedPtr<Layer> canvas);

		template<IsCoreAsset T, typename... Args>
		SharedPtr<T> CreateAsset(Args&&... args)
		{
			return m_AssetManager->Create<T>(std::forward<Args>(args)...);
		}

		template<IsCoreAsset T>
		SharedPtr<T> LoadAsset(const std::string& filePath)
		{
			return m_AssetManager->Load<T>(filePath);
		}

		template<IsCoreAsset T>
		SharedPtr<T> LoadAsset(const std::string& name, const std::string& filePath)
		{
			return m_AssetManager->Load<T>(name, filePath);
		}

		//void RegisterSystem(const SharedPtr<System>& system, Registry* registry);
		//void UnregisterSystem(const SharedPtr<System>& system, Registry* registry);
		//void UpdateSystems(TimeStep delta, Registry* registry);

		template<typename T>
		SharedPtr<T> GetSystem()
		{
			return m_SystemManager->GetSystem<T>();
		}

		void OnAttach();
		void OnDetach();
		void OnEvent(Event& event);

		void Run();
		bool IsRunning() const { return m_Running; }

		inline static Application& Instance() { return *s_Instance; }

        inline const Window& GetWindow() const { return *m_Window; }
		inline AssetManager& GetAssetManager() { return *m_AssetManager; }
		inline const AssetManager& GetAssetManager() const { return *m_AssetManager; }

		inline SystemManager& GetSystemManager() { return *m_SystemManager; }
		inline const SystemManager& GetSystemManager() const { return *m_SystemManager; }

	private:
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);
		bool OnKeyPressed(KeyPressedEvent& e);
		bool OnKeyReleased(KeyReleasedEvent& e);
		bool OnKeyRepeat(KeyRepeatEvent& e);
		bool OnMousePressed(MousePressedEvent& e);
		bool OnMouseReleased(MouseReleasedEvent& e);
		bool OnMouseScrolled(MouseScrolledEvent& e);
		bool OnMouseMoved(MouseMovedEvent& e);

	private:
		ScopedPtr<Window> m_Window;
		std::string m_Name;
		bool m_Running = true;

		ScopedPtr<AssetManager> m_AssetManager;
		ScopedPtr<SystemManager> m_SystemManager;

		LayerStack m_LayerStack;
		ScopedPtr<ImGuiLayer> m_ImGuiLayer;

		static Application* s_Instance;
	};

	ScopedPtr<Application> CreateApplication();
}

