#include "EditorLayer.h"
#include "Panels/SceneHierarchyPanel.h"
#include "Panels/InspectorPanel.h"
#include "Panels/AssetManagerPanel.h"

#include <random>

namespace Ember {

	EditorLayer::EditorLayer()
		: Layer("Ember Forge"), m_Context({ SharedPtr<Scene>::Create("DefaultScene"), Entity()})
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
			panel->SetContext(&m_Context);

		// Editor Camera Setup
		m_Camera = EditorCamera(65.0f, 1.778f, 0.1f, 500.0f);
		m_Camera.SetFocalPoint(Vector3f(0.0f, 0.0f, 0.0f));
		m_Camera.SetPitch(Math::Radians(30.0f));
		m_Camera.SetYaw(Math::Radians(45.0f));
		m_Camera.SetDistance(6.0f);

		// Output Framebuffer
		FramebufferSpecification specs;
		specs.Width = 800;
		specs.Height = 600;
		specs.AttachmentSpecs = {
			FramebufferTextureFormat::RGBA8,
			FramebufferTextureFormat::DEPTH24STENCIL8
		};
		m_OutputFramebuffer = Framebuffer::Create(specs);

		// -----------------------------------------------------------------
		// Default Cube
		// -----------------------------------------------------------------
		//auto cube = m_Context.ActiveScene->AddEntity();
		//MeshComponent meshComponent = { PrimitiveGenerator::CreateCube() };
		//cube.AttachComponent(meshComponent);
		MaterialComponent matComponent = { GetAsset<Material>(Constants::Assets::StandardGeometryMat)};
		//cube.AttachComponent(matComponent);

		// -----------------------------------------------------------------
		// Model
		// -----------------------------------------------------------------

		//auto satelliteAsset = LoadAsset<Ember::Model>("Satellite", "Sandbox/assets/models/Cube.obj");	// This one worked with lighting
		auto satelliteAsset = LoadAsset<Ember::Model>("Satellite", "Sandbox/assets/models/satellite.obj");
		m_Context.ActiveScene->InstantiateModel(satelliteAsset);

		// Plane for testing
		auto quadMesh = Ember::PrimitiveGenerator::CreateQuad(35.0f, 35.0f);
		auto groundPlane = m_Context.ActiveScene->AddEntity();
		auto& groundTransform = groundPlane.GetComponent<Ember::TransformComponent>();
		groundTransform.Position = { 0.0f, -4.0f, 0.0f };
		groundTransform.Rotation = { -1.5708f, 0.0f, 0.0f };

		Ember::MeshComponent groundMeshComp = { quadMesh };
		groundPlane.AttachComponent(groundMeshComp);
		groundPlane.AttachComponent(matComponent);

		auto groundInstance = groundPlane.GetComponent<Ember::MaterialComponent>().GetInstanced();
		groundInstance->Set("u_Albedo", Ember::Vector3f(0.3f, 0.3f, 0.3f));
		groundInstance->Set("u_Roughness", 0.7f);

		// Choose Lights
		SetupDirectionalLights();
	}

	void EditorLayer::OnDetach()
	{

	}

	void EditorLayer::OnEvent(Event& event)
	{
		// Handle events
		EB_CREATE_DISPATCHER(event);
		EB_DISPATCH_EVENT(MousePressedEvent, OnMouseClick);


		// Update camera
		if (m_Context.ActiveScene->GetSceneState() == SceneState::Edit)
			m_Camera.OnEvent(event);

		// Propagate events to panels
		for (auto& panel : m_Panels)
			panel->OnEvent(event);
	}

	void EditorLayer::OnUpdate(TimeStep delta)
	{
		m_OutputFramebuffer->Bind();

		RenderAction::SetViewport(0, 0, m_OutputFramebuffer->GetSpecification().Width, m_OutputFramebuffer->GetSpecification().Height);

		switch (m_Context.ActiveScene->GetSceneState())
		{
			case SceneState::Edit:
				m_Camera.OnUpdate(delta);
				m_Context.ActiveScene->OnUpdateEdit(delta, m_Camera);
				break;
			case SceneState::Play:
				m_Context.ActiveScene->OnUpdateRuntime(delta);
				break;
			case SceneState::Pause:
			default:
				EB_CORE_ASSERT(false, "Unhandled scene state!");
		}

		m_OutputFramebuffer->Unbind();

		RenderAction::SetClearColor(Vector4f(0.0f, 0.0f, 0.0f, 1.0f));
		RenderAction::Clear(RendererAPI::RenderBit::Color);
	}

	void EditorLayer::OnImGuiRender(TimeStep delta)
	{
		//ImGui::ShowDemoWindow();

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

			// Save view port info for mouse picking and viewport resizing
			m_ViewportHovered = ImGui::IsWindowHovered();
			
			auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
			auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
			auto viewportOffset = ImGui::GetWindowPos(); // Includes tab bar height

			m_ViewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
			m_ViewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

			ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
			if (m_ViewportSize.x != viewportPanelSize.x || m_ViewportSize.y != viewportPanelSize.y)
			{
				m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };
				m_OutputFramebuffer->ViewportResize((unsigned int)m_ViewportSize.x, (unsigned int)m_ViewportSize.y);
				m_Context.ActiveScene->OnViewportResize((unsigned int)m_ViewportSize.x, (unsigned int)m_ViewportSize.y);
				m_Camera.SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
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
		auto lightEntity = m_Context.ActiveScene->AddEntity();
		lightEntity.GetComponent<TransformComponent>().Position = Vector3f(0.0f, 20.0f, 0.0f);

		DirectionalLightComponent dirLightComp = { Vector3f(1.0f, -0.8f, -0.25f), Vector3f(1.0f, 0.8f, 0.8f), 5.0f };
		lightEntity.AttachComponent(dirLightComp);
	}

	bool EditorLayer::OnMouseClick(MousePressedEvent& e)
	{
		if (e.GetMouseButton() == MouseButton::Left && m_ViewportHovered)
		{
			auto [mx, my] = ImGui::GetMousePos();

			// Subtract the top-left corner of the viewport to get local coordinates
			mx -= m_ViewportBounds[0].x;
			my -= m_ViewportBounds[0].y;

			// Calculate viewport size from bounds
			Vector2f viewportSize = m_ViewportBounds[1] - m_ViewportBounds[0];

			// Flip the Y-Axis (ImGui is Top-Left origin, OpenGL is Bottom-Left origin)
			my = viewportSize.y - my;

			int mouseX = (int)mx;
			int mouseY = (int)my;

			// Ensure we are inside the image
			if (mouseX >= 0 && mouseY >= 0 && mouseX < (int)viewportSize.x && mouseY < (int)viewportSize.y)
			{
				m_Context.SelectedEntity = m_Context.ActiveScene->GetEntityAtPixel(mouseX, mouseY);
			}
		}

		return false;
	}
}
