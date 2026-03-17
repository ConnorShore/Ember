#include "ModelTestLayer.h"
#include "CameraController3D.h"

#include <random>

ModelTestLayer::ModelTestLayer()
	: Layer("Deferred Shading Test Layer"),
	m_MainScene(Ember::SharedPtr<Ember::Scene>::Create("Scene1"))
{
}

ModelTestLayer::~ModelTestLayer()
{
}

void ModelTestLayer::OnAttach()
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
	m_DefaultLightCubeMaterial = GetAsset<Ember::Material>(Ember::Constants::Assets::StandardUnlitMat);


	// ------------------------------------------------------------------
	// Ground plane — large quad rotated to lie flat beneath the spheres
	// ------------------------------------------------------------------
	auto quadMesh = Ember::PrimitiveGenerator::CreateQuad(35.0f, 35.0f);
	auto groundPlane = m_MainScene->AddEntity();
	auto& groundTransform = groundPlane.GetComponent<Ember::TransformComponent>();
	groundTransform.Position = { 0.0f, -10.0f, 0.0f };
	groundTransform.Rotation = { -1.5708f, 0.0f, 0.0f };

	Ember::MeshComponent groundMeshComp = { quadMesh };
	groundPlane.AttachComponent(groundMeshComp);

	Ember::MaterialComponent groundMatComp = { m_DefaultMaterial };
	groundPlane.AttachComponent(groundMatComp);

	auto groundInstance = groundPlane.GetComponent<Ember::MaterialComponent>().GetInstanced();
	groundInstance->Set("u_Albedo", Ember::Vector3f(0.3f, 0.3f, 0.3f));
	groundInstance->Set("u_Roughness", 0.7f);

	// Add our cube as an entity with deferred rendering components

	auto testCube = LoadAsset<Ember::Model>("TestCube", "Sandbox/assets/models/Cube.obj");

	if (testCube)
	{
		const auto& meshes = testCube->GetAllMeshes();
		EB_CORE_INFO("Successfully loaded model: {0}", testCube->GetName());
		EB_CORE_INFO("Total Meshes: {0}", meshes.size());
	}
	else
	{
		EB_CORE_INFO("Failed to load test cube!");
	}

	// -----------------------------------------------------------------
	// Model
	// -----------------------------------------------------------------
	Ember::MaterialComponent matTest = { m_DefaultMaterial };

	//auto satelliteAsset = LoadAsset<Ember::Model>("Satellite", "Sandbox/assets/models/Cube.obj");	// This one worked with lighting
	auto satelliteAsset = LoadAsset<Ember::Model>("Satellite", "Sandbox/assets/models/satellite.obj");
	m_Satellite = m_MainScene->InstantiateModel(satelliteAsset);

	// ------------------------------------------------------------------
	// Camera
	// ------------------------------------------------------------------
	m_CameraEntity = m_MainScene->AddEntity();
	auto& camTransform = m_CameraEntity.GetComponent<Ember::TransformComponent>();
	camTransform.Position = { 0.0f, 0.0f, 25.0f };

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
	SetupStandardLights();
	//SetupRandomLights();
}

void ModelTestLayer::OnDetach()
{

}

void ModelTestLayer::OnUpdate(Ember::TimeStep delta)
{
	auto& transform = m_Satellite.GetComponent<Ember::TransformComponent>();
	transform.Rotation.y += 1.0f * delta;

	// Rotate the "Left Wing" child entity in the opposite direction to demonstrate hierarchical transforms
	auto leftWing = m_Satellite.FindEntityInHierarchy("Wing_Left");
	auto& leftWingTransform = leftWing.GetComponent<Ember::TransformComponent>();
	leftWingTransform.Rotation.x += 1.0f * delta;

	m_Framebuffer->Bind();

	Ember::RenderAction::SetViewport(0, 0, m_Framebuffer->GetSpecification().Width, m_Framebuffer->GetSpecification().Height);

	m_MainScene->OnUpdate(delta);

	m_Framebuffer->Unbind();

	Ember::RenderAction::SetClearColor(Ember::Vector4f(0.0f, 0.0f, 0.0f, 1.0f));
	Ember::RenderAction::Clear(Ember::RendererAPI::RenderBit::Color);
}

void ModelTestLayer::OnImGuiRender(Ember::TimeStep delta)
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

void ModelTestLayer::SetupStandardLights()
{
	struct LightDesc {
		Ember::Vector3f position;
		Ember::Vector3f color;
		float intensity;
		float radius;
	};

	LightDesc lights[] = {
		// Key  – upper-front-right, warm white
		{ {  10.0f,  10.0f,  15.0f }, { 1.0f, 0.95f, 0.9f  }, 500.0f, 50.0f },
		// Fill – upper-front-left, cool white, dimmer
		{ { -10.0f,   8.0f,  12.0f }, { 0.9f, 0.95f, 1.0f  }, 350.0f, 50.0f },
		// Rim  – upper-back-right
		{ {  12.0f,   6.0f, -10.0f }, { 1.0f, 1.0f,  1.0f  }, 400.0f, 50.0f },
		// Rim  – upper-back-left
		{ { -12.0f,   6.0f, -10.0f }, { 1.0f, 1.0f,  1.0f  }, 400.0f, 50.0f },
	};

	auto lightCubeMesh = Ember::PrimitiveGenerator::CreateCube(1.0f);
	for (auto& ld : lights)
	{
		auto lightEntity = m_MainScene->AddEntity();
		Ember::PointLightComponent plComp = { ld.color, ld.intensity, ld.radius };
		lightEntity.AttachComponent(plComp);
		auto& lt = lightEntity.GetComponent<Ember::TransformComponent>();
		lt.Position = ld.position;
		lt.Scale = { 0.3f, 0.3f, 0.3f };

		Ember::MeshComponent lightCubeMeshComp = { lightCubeMesh };
		lightEntity.AttachComponent(lightCubeMeshComp);

		Ember::MaterialComponent lightCubeMatComp = { m_DefaultLightCubeMaterial };
		lightEntity.AttachComponent(lightCubeMatComp);

		auto lightCubeInstance = lightEntity.GetComponent<Ember::MaterialComponent>().GetInstanced();
		lightCubeInstance->Set("u_Color", ld.color);
	}
}

void ModelTestLayer::SetupRandomLights()
{
	// Setup random number generation
	std::random_device rd;
	std::mt19937 gen(rd());

	// Spread lights across an area roughly matching your 7x7 grid
	std::uniform_real_distribution<float> randomX(-15.0f, 25.0f);
	std::uniform_real_distribution<float> randomY(-15.0f, 15.0f);
	std::uniform_real_distribution<float> randomZ(2.0f, 20.0f);  // Keep them slightly above the ground plane

	// Randomize color and keep them reasonably bright
	std::uniform_real_distribution<float> randomColor(0.3f, 1.0f);

	// Drop intensity way down so 1000 lights don't blind the camera
	std::uniform_real_distribution<float> randomIntensity(5.0f, 50.0f);

	int numLights = 256;
	auto lightCubeMesh = Ember::PrimitiveGenerator::CreateCube(1.0f);

	for (int i = 0; i < numLights; i++)
	{
		auto lightEntity = m_MainScene->AddEntity();

		// Generate random values for this specific light
		Ember::Vector3f color = { randomColor(gen), randomColor(gen), randomColor(gen) };
		float intensity = randomIntensity(gen);
		float radius = 15.0f; // Keep the radius somewhat constrained

		Ember::PointLightComponent plComp = { color, intensity, radius };
		lightEntity.AttachComponent(plComp);

		auto& lt = lightEntity.GetComponent<Ember::TransformComponent>();
		lt.Position = { randomX(gen), randomY(gen), randomZ(gen) };
		lt.Scale = { 0.1f, 0.1f, 0.1f }; // Make the debug cubes tiny

		Ember::MeshComponent lightCubeMeshComp = { lightCubeMesh };
		lightEntity.AttachComponent(lightCubeMeshComp);

		Ember::MaterialComponent lightCubeMatComp = { m_DefaultLightCubeMaterial }; // Ensure this matches your material variable name
		lightEntity.AttachComponent(lightCubeMatComp);

		auto lightCubeInstance = lightEntity.GetComponent<Ember::MaterialComponent>().GetInstanced();

		// Set the physical cube to match the color of the light it emits!
		lightCubeInstance->Set("u_Color", color);
	}
}
