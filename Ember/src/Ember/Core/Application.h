#pragma once

#include "Window.h"
#include "LayerStack.h"
#include "ScopedPointer.h"

#include "Ember/Event/Event.h"
#include "Ember/Event/WindowEvent.h"
#include "Ember/Event/KeyEvent.h"
#include "Ember/Event/MouseEvent.h"
#include "Ember/ImGui/ImGuiLayer.h"

#include "Ember/Render/Shader.h"
#include "Ember/Render/Texture.h"

namespace Ember {

	class Application
	{
	public:
		Application();
		virtual ~Application();

		void PushLayer(ScopedPtr<Layer> layer);
		void PushCanvasLayer(ScopedPtr<Layer> canvas);

		void RegisterShader(const std::string& filePath);
		SharedPtr<Shader> GetShader(const std::string& name);

		void RegisterTexture(const std::string& filePath);
		SharedPtr<Texture> GetTexture(const std::string& name);

		void OnAttach();
		void OnDetatch();
		void OnEvent(Event& event);

		void Run();
		bool IsRunning() const { return m_Running; }

		inline static Application& Instance() { return *s_Instance; }

		inline const Window& GetWindow() const { return *m_Window; }

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
		bool m_Running = true;

		LayerStack m_LayerStack;
		ScopedPtr<ImGuiLayer> m_ImGuiLayer;

		// TODO: Probably move these to some AssetManager or something
		ScopedPtr<ShaderLibrary> m_ShaderLibrary;
		ScopedPtr<TextureLibrary> m_TextureLibrary;

		static Application* s_Instance;
	};

	Application* CreateApplication();
}

