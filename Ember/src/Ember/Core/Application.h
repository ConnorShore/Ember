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
#include "Ember/Render/Mesh.h"
#include "Ember/Render/Material.h"

namespace Ember {

	class Application
	{
	public:
		Application();
		virtual ~Application();

		void PushLayer(ScopedPtr<Layer> layer);
		void PushCanvasLayer(ScopedPtr<Layer> canvas);

		const SharedPtr<Shader>& RegisterShader(const std::string& filePath);
		const SharedPtr<Shader>& GetShader(const std::string& name);

		const SharedPtr<Texture>& RegisterTexture(const std::string& filePath);
		const SharedPtr<Texture>& GetTexture(const std::string& name);

		const SharedPtr<Mesh>& RegisterMesh(const std::string& filePath);
		const SharedPtr<Mesh>& GetMesh(const std::string& name);

		const SharedPtr<Material>& RegisterMaterial(const std::string& name, const SharedPtr<Shader>& shader);
		const SharedPtr<Material>& RegisterMaterial(const std::string& name, const SharedPtr<Shader>& shader, std::initializer_list<MaterialUniform> uniforms);
		const SharedPtr<MaterialInstance>& RegisterMaterial(const std::string& name, const SharedPtr<Material>& material);
		const SharedPtr<MaterialBase>& GetMaterial(const std::string& name);

		void OnAttach();
		void OnDetach();
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
		ScopedPtr<MeshLibrary> m_MeshLibrary;
		ScopedPtr<MaterialLibrary> m_MaterialLibrary;

		static Application* s_Instance;
	};

	ScopedPtr<Application> CreateApplication();
}

