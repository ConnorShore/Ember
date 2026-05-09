#include "RuntimeLayer.h"
#include <Ember/Core/ProjectManager.h>
#include <Ember/Scene/SceneSerializer.h>
#include <Ember/Render/RenderAction.h>

namespace Ember {

	void RuntimeLayer::OnAttach()
	{
		// Load the exported project configuration
		std::string projectPath = "project.ebproj";	// Default to looking for the project file in the current working directory
		auto& app = Application::Instance();
		if (app.GetCommandLineArgsCount() > 1)
			projectPath = app.GetCommandLineArg(1);

		ProjectManager::LoadProject(projectPath);

		// 2. Create and deserialize the startup scene
		m_ActiveScene = SharedPtr<Scene>::Create("Runtime Scene");
		SceneSerializer serializer(m_ActiveScene);
		serializer.Deserialize(ProjectManager::GetActive()->GetStartScenePath().string());

		// 3. Size the camera and render passes to the actual OS Window, not an ImGui panel!
		auto& window = Application::Instance().GetWindow();
		m_ActiveScene->OnViewportResize(window.GetWidth(), window.GetHeight());

		// 4. Start the game!
		m_ActiveScene->OnRuntimeStart();

		// 5. Lock the cursor immediately for a First-Person game
		Input::SetCursorMode(CursorMode::Locked);
	}

	void RuntimeLayer::OnDetach()
	{
		m_ActiveScene->OnRuntimeStop();
	}

	void RuntimeLayer::OnUpdate(TimeStep delta)
	{
		// 1. Clear the main OS window backbuffer directly
		RenderAction::SetClearColor(Vector4f(0.0f, 0.0f, 0.0f, 1.0f));
		RenderAction::Clear(RendererAPI::RenderBit::Color);

		// 2. Set the GL viewport to the full window so the render pipeline reads the correct
		//    dimensions from glGetIntegerv(GL_VIEWPORT, ...) inside ExecuteRenderPipeline.
		//    The editor does this via its output framebuffer before calling OnUpdateRuntime;
		//    the standalone runtime must do it explicitly against the OS window.
		auto& window = Application::Instance().GetWindow();
		RenderAction::SetViewport(0, 0, window.GetWidth(), window.GetHeight());

		// 3. Step the game loop! 
		// (This internally handles physics, scripts, AI, and calls your RenderSystem)
		m_ActiveScene->OnUpdateRuntime(delta);

		// 3. Handle exiting the game
		// In a real game you'd probably open a pause menu here, but for now we just quit.
		if (Input::IsKeyPressed(KeyCode::Escape))
		{
			// Assuming your Application has a Close() or Quit() method to break the while loop
			Input::SetCursorMode(CursorMode::Normal);
		}
	}

	bool RuntimeLayer::OnWindowResize(WindowResizeEvent& e)
	{
		m_ActiveScene->OnViewportResize(e.GetWidth(), e.GetHeight());
		Application::Instance().GetSystemManager().GetSystem<RenderSystem>()->OnViewportResize(e.GetWidth(), e.GetHeight());
		return false;
	}

	void RuntimeLayer::OnEvent(Event& event)
	{
		EB_CREATE_DISPATCHER(event);
		EB_DISPATCH_EVENT(WindowResizeEvent, OnWindowResize);

		m_ActiveScene->OnEvent(event);
	}
}