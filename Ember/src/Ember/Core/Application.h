#pragma once

#include "Window.h"
#include "LayerStack.h"
#include "ScopedPointer.h"

#include "Ember/Event/Event.h"
#include "Ember/Event/WindowEvent.h"
#include "Ember/Event/KeyEvent.h"
#include "Ember/Event/MouseEvent.h"
#include "Ember/ImGui/ImGuiLayer.h"

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

		void OnAttach();
		void OnDetach();
		void OnEvent(Event& event);

		void Run();
		bool IsRunning() const { return m_Running; }

		inline static Application& Instance() { return *s_Instance; }

		inline const Window& GetWindow() const { return *m_Window; }
		inline const AssetManager& GetAssetManager() const { return *m_AssetManager; }

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
		std::string m_Name;
		bool m_Running = true;

		LayerStack m_LayerStack;
		ScopedPtr<ImGuiLayer> m_ImGuiLayer;
		ScopedPtr<AssetManager> m_AssetManager;

		static Application* s_Instance;
	};

	ScopedPtr<Application> CreateApplication();
}

