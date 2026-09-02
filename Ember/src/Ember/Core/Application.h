#pragma once

#include "ApplicationSpecification.h"
#include "Window.h"
#include "LayerStack.h"
#include "ScopedPointer.h"
#include "SaveGameManager.h"

#include "Ember/Event/Event.h"
#include "Ember/Event/WindowEvent.h"
#include "Ember/Event/KeyEvent.h"
#include "Ember/Event/MouseEvent.h"
#include "Ember/ImGui/ImGuiLayer.h"

#include "Ember/Scene/SceneManager.h"

#include "Ember/ECS/System/SystemManager.h"

#include "Ember/Input/InputActionManager.h"

#include "Ember/Asset/AssetManager.h"
#include "Ember/Asset/Asset.h"

namespace Ember {

	class Application
	{
	public:
		Application(const ApplicationSpecification& applicationSpecs);
		virtual ~Application();

		void PushLayer(ScopedPtr<Layer> layer);
		void PushCanvasLayer(ScopedPtr<Layer> canvas);

		template<IsCoreAsset T, typename... Args>
		SharedPtr<T> CreateAsset(Args&&... args)
		{
			return m_AssetManager.Create<T>(std::forward<Args>(args)...);
		}

		template<IsCoreAsset T>
		SharedPtr<T> LoadAsset(const std::string& filePath)
		{
			return m_AssetManager.Load<T>(filePath);
		}

		template<IsCoreAsset T>
		SharedPtr<T> LoadAsset(const std::string& name, const std::string& filePath)
		{
			return m_AssetManager.Load<T>(name, filePath);
		}

		template<typename T>
		SharedPtr<T> GetSystem()
		{
			return m_SystemManager.GetSystem<T>();
		}

		void OnAttach();
		void OnDetach();
		void OnEvent(Event& event);

		void Run();
		void Close();
		bool IsRunning() const { return m_Running; }

		// Queues a QuitRequestedEvent for the end of the frame; unhandled, it closes the application.
		void RequestQuit();

		inline static Application& Instance() { return *s_Instance; }

        inline const Window& GetWindow() const { return *m_Window; }
		inline AssetManager& GetAssetManager() { return m_AssetManager; }
		inline const AssetManager& GetAssetManager() const { return m_AssetManager; }
		inline SceneManager& GetSceneManager() { return m_SceneManager; }
		inline SaveGameManager& GetSaveGameManager() { return m_SaveGameManager; }
		inline const SaveGameManager& GetSaveGameManager() const { return m_SaveGameManager; }

		inline ImGuiLayer& GetImGuiLayer() { return m_ImGuiLayer; }

		inline SystemManager& GetSystemManager() { return m_SystemManager; }
		inline InputActionManager& GetInputActionManager() { return m_InputActions; }
		inline const InputActionManager& GetInputActionManager() const { return m_InputActions; }
		inline const SystemManager& GetSystemManager() const { return m_SystemManager; }

		inline void SetCursorPosition(float x, float y) { m_Window->SetCursorPosition(x, y); }

		inline void SetCursorMode(CursorMode mode) { m_Window->SetCursorMode(mode); }
		inline CursorMode GetCursorMode() const { return m_Window->GetCursorMode(); }

		inline const ApplicationSpecification& GetSpecification() const { return m_Specification; }

		inline const int GetCommandLineArgsCount() const { return m_Specification.CommandLineArgsCount; }
		inline const char* GetCommandLineArg(int index) const
		{
			if (index < 0 || index >= m_Specification.CommandLineArgsCount)
				return nullptr;
			return m_Specification.CommandLineArgs[index];
		}

	private:
		void ProcessQuitRequest();

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
		ApplicationSpecification m_Specification;

		ScopedPtr<Window> m_Window;
		bool m_Running = true;
		bool m_QuitRequested = false;

		SystemManager m_SystemManager;
		AssetManager m_AssetManager;
		SceneManager m_SceneManager;

		// Declared before m_InputActions and constructed before the body, so it is guaranteed to
		// exist by the time ScriptEngine::Init() publishes `GameData` as a pointer to it.
		SaveGameManager m_SaveGameManager;
		InputActionManager m_InputActions;

		LayerStack m_LayerStack;
		ImGuiLayer m_ImGuiLayer;

		static Application* s_Instance;
	};

	ScopedPtr<Application> CreateApplication(int argc, char** argv);
}

