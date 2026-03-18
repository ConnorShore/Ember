#include "EditorLayer.h"
#include "CameraController3D.h"
#include "Panels/SceneHierarchyPanel.h"
#include "Panels/InspectorPanel.h"
#include "Panels/AssetManagerPanel.h"

#include <random>

namespace Ember {

	EditorLayer::EditorLayer()
		: Layer("Ember Forge"),
		m_ActiveScene(SharedPtr<Scene>::Create("DefaultScene"))
	{
	}

	EditorLayer::~EditorLayer()
	{
	}

	void EditorLayer::OnAttach()
	{
		// Add Panels
		m_Panels.push_back(SharedPtr<SceneHierarchyPanel>::Create());
		m_Panels.push_back(SharedPtr<InspectorPanel>::Create());
		m_Panels.push_back(SharedPtr<AssetManagerPanel>::Create());

		for (auto& panel : m_Panels)
			panel->SetContext(m_ActiveScene);


		// Output Framebuffer
		FramebufferSpecification specs;
		specs.Width = 800;
		specs.Height = 600;
		specs.AttachmentSpecs = {
			FramebufferTextureFormat::RGBA8,
			//FramebufferTextureFormat::RED_INTEGER,	// For mouse picking
			FramebufferTextureFormat::DEPTH24STENCIL8
		};
		m_OutputFramebuffer = Framebuffer::Create(specs);

		// -----------------------------------------------------------------
		// Default Cube
		// -----------------------------------------------------------------
		auto cube = m_ActiveScene->AddEntity();
		MeshComponent meshComponent = { PrimitiveGenerator::CreateCube() };
		cube.AttachComponent(meshComponent);
		MaterialComponent matComponent = { GetAsset<Material>(Constants::Assets::StandardGeometryMat)};
		cube.AttachComponent(matComponent);

		// ------------------------------------------------------------------
		// Camera (TODO: Make designated scene camera)
		// ------------------------------------------------------------------
		m_CameraEntity = m_ActiveScene->AddEntity();
		auto& camTransform = m_CameraEntity.GetComponent<TransformComponent>();
		camTransform.Position = { 0.0f, 0.0f, 10.0f };

		Camera camera;
		camera.SetViewportSize(
			Application::Instance().GetWindow().GetWidth(),
			Application::Instance().GetWindow().GetHeight());
		camera.SetProjectionType(Camera::ProjectionType::Perspective);
		camera.SetPerspective(65.0f, 0.1f, 200.0f);

		CameraComponent cameraComponent(camera, true);
		m_CameraEntity.AttachComponent(cameraComponent);
		m_CameraEntity.AttachComponent<ScriptComponent>().Bind<Camera3DController>();

		// Choose Lights
		SetupDirectionalLights();
	}

	void EditorLayer::OnDetach()
	{

	}

	void EditorLayer::OnEvent(Event& event)
	{
		// Handle events

		// Propagate events to panels
		for (auto& panel : m_Panels)
			panel->OnEvent(event);
	}

	void EditorLayer::OnUpdate(TimeStep delta)
	{
		m_OutputFramebuffer->Bind();

		RenderAction::SetViewport(0, 0, m_OutputFramebuffer->GetSpecification().Width, m_OutputFramebuffer->GetSpecification().Height);

		m_ActiveScene->OnUpdate(delta);

		m_OutputFramebuffer->Unbind();

		RenderAction::SetClearColor(Vector4f(0.0f, 0.0f, 0.0f, 1.0f));
		RenderAction::Clear(RendererAPI::RenderBit::Color);
	}

	void EditorLayer::OnImGuiRender(TimeStep delta)
	{
		ImGui::DockSpaceOverViewport();

		// FPS calculation (updated every 1 seconds to avoid rapid fluctuations)
		static float fps = 0.0f;
		static float fpsTimer = 0.0f;

		fpsTimer += delta.Seconds();
		if (fpsTimer >= 1.0f)
		{
			fps = 1.0f / delta.Seconds();
			fpsTimer = 0.0f;
		}

		// Menu Bar
		{
			ImGui::BeginMainMenuBar();

			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("New Scene", "Ctrl+N"))
				{
					EB_CORE_TRACE("New Scene menu item clicked!");
				}

				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		// Viewport
		{
			ImGui::Begin("Scene Viewport");
			ImGui::Text("FPS: %.1f", fps);

			ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

			if (m_ViewportSize.x != viewportPanelSize.x || m_ViewportSize.y != viewportPanelSize.y)
			{
				m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };
				m_OutputFramebuffer->ViewportResize((unsigned int)m_ViewportSize.x, (unsigned int)m_ViewportSize.y);
				m_ActiveScene->OnViewportResize((unsigned int)m_ViewportSize.x, (unsigned int)m_ViewportSize.y);
			}

			unsigned int textureID = m_OutputFramebuffer->GetColorAttachmentID(0);
			ImGui::Image((void*)textureID, ImVec2{ viewportPanelSize.x, viewportPanelSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

			ImGui::End();
		}

		// Render Panels
		for (auto& panel : m_Panels)
			panel->OnImGuiRender();
	}

	void EditorLayer::SetupDirectionalLights()
	{
		auto lightEntity = m_ActiveScene->AddEntity();
		lightEntity.GetComponent<TransformComponent>().Position = Vector3f(0.0f, 20.0f, 0.0f);

		DirectionalLightComponent dirLightComp = { Vector3f(1.0f, -0.8f, -0.25f), Vector3f(1.0f, 0.8f, 0.8f), 5.0f };
		lightEntity.AttachComponent(dirLightComp);
	}

}
