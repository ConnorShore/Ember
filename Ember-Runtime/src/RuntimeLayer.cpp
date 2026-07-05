#include "RuntimeLayer.h"
#include <Ember/Core/ProjectManager.h>
#include <Ember/Scene/SceneSerializer.h>
#include <Ember/Render/RenderAction.h>
#include <Ember/Asset/Serializers/AssetSerializationMode.h>

#include <imgui/imgui.h>

namespace Ember {

	void RuntimeLayer::OnAttach()
	{
		const bool developerRawMode = Application::Instance().GetCommandLineArgsCount() >= 4;
		AssetSerializationMode::SetRuntimeLoadTier(
			developerRawMode ? RuntimeAssetLoadTier::ForceSourceYaml : RuntimeAssetLoadTier::ForceCookedBinary);

		// Load the exported project configuration
		std::string projectPath = "project.ebproj";	// Default to looking for the project file in the current working directory
		auto& app = Application::Instance();
		if (app.GetCommandLineArgsCount() > 1)
			projectPath = app.GetCommandLineArg(1);

		ProjectManager::LoadProject(projectPath);

		// 2. Create and deserialize the startup scene
		auto runtimeScene = SharedPtr<Scene>::Create("RuntimeScene", "");
		Application::Instance().GetSceneManager().SetActiveScene(runtimeScene);
		SceneSerializer serializer(runtimeScene);
		if (AssetSerializationMode::GetRuntimeLoadTier() == RuntimeAssetLoadTier::ForceCookedBinary)
			serializer.DeserializeCooked(ProjectManager::GetActive()->GetStartScenePath().string());
		else
			serializer.Deserialize(ProjectManager::GetActive()->GetStartScenePath().string());

		// 3. Size the camera and render passes to the actual OS Window, not an ImGui panel!
		auto& window = Application::Instance().GetWindow();
		runtimeScene->OnViewportResize(window.GetWidth(), window.GetHeight());

		// 4. Start the game!
		runtimeScene->OnRuntimeStart();

		// 5. Lock the cursor immediately for a First-Person game
		Input::SetCursorMode(CursorMode::Locked);

		// Ensure active scene starts at 0
		ProjectManager::GetActive()->ResetSceneIndex();
	}

	void RuntimeLayer::OnDetach()
	{
		Application::Instance().GetSceneManager().GetActiveScene()->OnRuntimeStop();
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
		Application::Instance().GetSceneManager().GetActiveScene()->OnUpdateRuntime(delta);

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
		Application::Instance().GetSceneManager().GetActiveScene()->OnViewportResize(e.GetWidth(), e.GetHeight());
		Application::Instance().GetSystemManager().GetSystem<RenderSystem>()->OnViewportResize(e.GetWidth(), e.GetHeight());
		return false;
	}

	bool RuntimeLayer::OnKeyPressed(KeyPressedEvent& e)
	{
		if (e.GetKeyCode() == KeyCode::F3)
		{
			m_ShowFPSOverlay = !m_ShowFPSOverlay;
			return true;
		}
		return false;
	}

	void RuntimeLayer::OnImGuiRender(TimeStep delta)
	{
		if (!m_ShowFPSOverlay)
			return;

		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking |
			ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs;

		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImVec2 windowPos = ImVec2(viewport->WorkPos.x + 10.0f, viewport->WorkPos.y + 10.0f);
		ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, ImVec2(0.0f, 0.0f));
		ImGui::SetNextWindowBgAlpha(0.35f);

		if (ImGui::Begin("##RuntimeFPSOverlay", nullptr, windowFlags))
		{
			ImGui::Text("FPS: %.1f", CalculateFPS(delta));
		}
		ImGui::End();
	}

	float RuntimeLayer::CalculateFPS(TimeStep delta)
	{
		static float fpsTimer = 0.0f;
		static float fps = 0.0f;
		static int frameCount = 0;

		fpsTimer += delta.Seconds();
		frameCount++;

		if (fpsTimer >= 1.0f)
		{
			fps = (float)frameCount / fpsTimer;
			fpsTimer = 0.0f;
			frameCount = 0;
		}

		return fps;
	}

	void RuntimeLayer::OnEvent(Event& event)
	{
		EB_CREATE_DISPATCHER(event);
		EB_DISPATCH_EVENT(WindowResizeEvent, OnWindowResize);
		EB_DISPATCH_EVENT(KeyPressedEvent, OnKeyPressed);

		Application::Instance().GetSceneManager().GetActiveScene()->OnEvent(event);
	}
}