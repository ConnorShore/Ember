#include "RuntimeLayer.h"
#include <Ember/Core/ProjectManager.h>
#include <Ember/Scene/SceneSerializer.h>
#include <Ember/Render/RenderAction.h>

namespace Ember {

	void RuntimeLayer::OnAttach()
	{
		// 1. Load the exported project configuration
		// In a real build, this path will be relative to the executable
		ProjectManager::LoadProject("C:\\Users\\cjsho\\Desktop\\NewProject\\NewProject.ebproj");

		//auto& assetManager = Application::Instance().GetAssetManager();
		//assetManager.SetEngineAssetDirectory("Ember-Forge/assets");
		//assetManager.SetProjectAssetDirectory(ProjectManager::GetActive()->GetAssetDirectory());
		//assetManager.LoadDefaults();

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
		Input::SetCursorMode(CursorMode::Normal);
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

		// 2. Step the game loop! 
		// (This internally handles physics, scripts, AI, and calls your RenderSystem)
		m_ActiveScene->OnUpdateRuntime(delta);

		// 3. Handle exiting the game
		// In a real game you'd probably open a pause menu here, but for now we just quit.
		if (Input::IsKeyPressed(KeyCode::Escape))
		{
			// Assuming your Application has a Close() or Quit() method to break the while loop
			Application::Instance().Close();
		}
	}

	void RuntimeLayer::OnEvent(Event& event)
	{
		// Pass window resize events down to the scene so the camera aspect ratio updates
		EB_CREATE_DISPATCHER(event);
		// EB_DISPATCH_EVENT(WindowResizeEvent, ...); 

		m_ActiveScene->OnEvent(event);
	}
}