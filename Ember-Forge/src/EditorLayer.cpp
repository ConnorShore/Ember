#include "EditorLayer.h"
#include "CameraController3D.h"

#include <random>

EditorLayer::EditorLayer()
	: Layer("Ember Forge"),
	m_MainScene(Ember::SharedPtr<Ember::Scene>::Create("DefaultScene"))
{
}

EditorLayer::~EditorLayer()
{
}

void EditorLayer::OnAttach()
{
	// Framebuffer
	Ember::FramebufferSpecification specs;
	specs.Width = 800;
	specs.Height = 600;
	specs.AttachmentSpecs = {
		Ember::FramebufferTextureFormat::RGBA8,
		Ember::FramebufferTextureFormat::DEPTH24STENCIL8
	};
	m_Framebuffer = Ember::Framebuffer::Create(specs);

	// Materials
	m_DefaultMaterial = GetAsset<Ember::Material>(Ember::Constants::Assets::StandardGeometryMat);

	// -----------------------------------------------------------------
	// Default Cube
	// -----------------------------------------------------------------
	auto cube = m_MainScene->AddEntity();
	Ember::MeshComponent meshComponent = { Ember::PrimitiveGenerator::CreateCube() };
	cube.AttachComponent(meshComponent);
	Ember::MaterialComponent matComponent = { m_DefaultMaterial };
	cube.AttachComponent(matComponent);

	// ------------------------------------------------------------------
	// Camera (TODO: Make designated scene camera)
	// ------------------------------------------------------------------
	m_CameraEntity = m_MainScene->AddEntity();
	auto& camTransform = m_CameraEntity.GetComponent<Ember::TransformComponent>();
	camTransform.Position = { 0.0f, 0.0f, 10.0f };

	Ember::Camera camera;
	camera.SetViewportSize(
		Ember::Application::Instance().GetWindow().GetWidth(),
		Ember::Application::Instance().GetWindow().GetHeight());
	camera.SetProjectionType(Ember::Camera::ProjectionType::Perspective);
	camera.SetPerspective(65.0f, 0.1f, 200.0f);

	Ember::CameraComponent cameraComponent(camera, true);
	m_CameraEntity.AttachComponent(cameraComponent);
	m_CameraEntity.AttachComponent<Ember::ScriptComponent>().Bind<Camera3DController>();

	// Choose Lights
	SetupDirectionalLights();
}

void EditorLayer::OnDetach()
{

}

void EditorLayer::OnUpdate(Ember::TimeStep delta)
{
	m_Framebuffer->Bind();

	Ember::RenderAction::SetViewport(0, 0, m_Framebuffer->GetSpecification().Width, m_Framebuffer->GetSpecification().Height);

	m_MainScene->OnUpdate(delta);

	m_Framebuffer->Unbind();

	Ember::RenderAction::SetClearColor(Ember::Vector4f(0.0f, 0.0f, 0.0f, 1.0f));
	Ember::RenderAction::Clear(Ember::RendererAPI::RenderBit::Color);
}

void EditorLayer::OnImGuiRender(Ember::TimeStep delta)
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

	// Viewport
	{
		ImGui::Begin("Scene Viewport");
		ImGui::Text("FPS: %.1f", fps);

		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

		if (m_ViewportSize.x != viewportPanelSize.x || m_ViewportSize.y != viewportPanelSize.y)
		{
			m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };
			m_Framebuffer->ViewportResize((unsigned int)m_ViewportSize.x, (unsigned int)m_ViewportSize.y);
			m_MainScene->OnViewportResize((unsigned int)m_ViewportSize.x, (unsigned int)m_ViewportSize.y);
		}

		unsigned int textureID = m_Framebuffer->GetColorAttachmentID(0);
		ImGui::Image((void*)textureID, ImVec2{ viewportPanelSize.x, viewportPanelSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

		ImGui::End();
	}

	// Editor Panel
	{
		ImGui::Begin("Interactive Model Editor");

		ImGui::Text("Interactive Model");
		ImGui::Separator();


		ImGui::End();
	}
}

void EditorLayer::SetupDirectionalLights()
{
	auto lightEntity = m_MainScene->AddEntity();
	lightEntity.GetComponent<Ember::TransformComponent>().Position = Ember::Vector3f(0.0f, 20.0f, 0.0f);

	Ember::DirectionalLightComponent dirLightComp = { Ember::Vector3f(1.0f, -0.8f, -0.25f), Ember::Vector3f(1.0f, 0.8f, 0.8f), 5.0f };
	lightEntity.AttachComponent(dirLightComp);
}