#include "ebpch.h"
#include "PBRTestLayer.h"

#include <imgui/imgui.h>

// ---------------------------------------------------------------------------
// Camera controller:  WASD = translate,  Arrow keys = rotate
// ---------------------------------------------------------------------------
class Camera3DController : public Ember::Behavior
{
public:
	void OnUpdate(Ember::TimeStep delta) override
	{
		float speed    = 5.0f * delta;
		float rotSpeed = 1.5f * delta;

		// Movement (WASD)
		if (Ember::Input::IsKeyPressed(Ember::KeyCode::W))
			Transform().Position += Transform().GetForward() * speed;
		if (Ember::Input::IsKeyPressed(Ember::KeyCode::S))
			Transform().Position -= Transform().GetForward() * speed;
		if (Ember::Input::IsKeyPressed(Ember::KeyCode::D))
			Transform().Position += Transform().GetRight() * speed;
		if (Ember::Input::IsKeyPressed(Ember::KeyCode::A))
			Transform().Position -= Transform().GetRight() * speed;

		// Rotation (Arrow keys)
		if (Ember::Input::IsKeyPressed(Ember::KeyCode::Right))
			Transform().Rotation.y -= rotSpeed;
		if (Ember::Input::IsKeyPressed(Ember::KeyCode::Left))
			Transform().Rotation.y += rotSpeed;
		if (Ember::Input::IsKeyPressed(Ember::KeyCode::Up))
			Transform().Rotation.x += rotSpeed;
		if (Ember::Input::IsKeyPressed(Ember::KeyCode::Down))
			Transform().Rotation.x -= rotSpeed;
	}
};

// ---------------------------------------------------------------------------
PBRTestLayer::PBRTestLayer()
	: Layer("3D Test Layer"), m_MainScene(Ember::SharedPtr<Ember::Scene>::Create("Scene1"))
{
}

PBRTestLayer::~PBRTestLayer()
{
}

void PBRTestLayer::OnAttach()
{
	auto mesh      = Ember::PrimitiveGenerator::CreateSphere(1.0f, 64, 64);
	auto pbrShader = RegisterShader("assets/shaders/pbr.glsl");

	// Base PBR material (defaults – overridden per-instance)
	auto pbrMaterial = RegisterMaterial("pbrMaterial", pbrShader, {
		{ "u_Albedo",    Ember::Vector3f(0.5f, 0.5f, 0.5f) },
		{ "u_Metallic",  0.0f },
		{ "u_Roughness", 0.5f },
		{ "u_AO",        1.0f },
		{ "u_Texture",   Ember::Renderer3D::GetWhiteTexture() }
	});

	// ------------------------------------------------------------------
	// Parameter grid:  7 columns (roughness) × 7 rows (metallic)
	// Roughness ramps left-to-right   0.05 → 1.0
	// Metallic  ramps bottom-to-top   0.0  → 1.0
	// ------------------------------------------------------------------
	constexpr int   cols    = 7;
	constexpr int   rows    = 7;
	constexpr float spacing = 2.5f;

	for (int row = 0; row < rows; row++)
	{
		float metallic = (float)row / (float)(rows - 1);

		for (int col = 0; col < cols; col++)
		{
			float roughness = 0.05f + ((float)col / (float)(cols - 1)) * 0.95f;

			auto sphere = m_MainScene->AddEntity();
			auto& transform = sphere.GetComponent<Ember::TransformComponent>();
			transform.Position = {
				(col - cols / 2) * spacing,
				(row - rows / 2) * spacing,
				0.0f
			};

			Ember::MeshComponent meshComp = { mesh };
			sphere.AttachComponent(meshComp);

			Ember::MaterialComponent matComp = { pbrMaterial };
			sphere.AttachComponent(matComp);

			auto instance = sphere.GetComponent<Ember::MaterialComponent>().GetInstanced();
			instance->Set("u_Albedo",    Ember::Vector3f(0.5f, 0.0f, 0.0f));
			instance->Set("u_Metallic",  metallic);
			instance->Set("u_Roughness", roughness);
		}
	}

	// ------------------------------------------------------------------
	// Ground plane — large quad rotated to lie flat beneath the spheres
	// ------------------------------------------------------------------
	auto quadMesh = Ember::PrimitiveGenerator::CreateQuad(35.0f, 35.0f);
	auto groundPlane = m_MainScene->AddEntity();
	auto& groundTransform = groundPlane.GetComponent<Ember::TransformComponent>();
	groundTransform.Position = { 0.0f, -(rows / 2) * spacing - 1.5f, 0.0f };
	groundTransform.Rotation = { -1.5708f, 0.0f, 0.0f };

	Ember::MeshComponent groundMeshComp = { quadMesh };
	groundPlane.AttachComponent(groundMeshComp);

	Ember::MaterialComponent groundMatComp = { pbrMaterial };
	groundPlane.AttachComponent(groundMatComp);

	auto groundInstance = groundPlane.GetComponent<Ember::MaterialComponent>().GetInstanced();
	groundInstance->Set("u_Albedo",    Ember::Vector3f(0.3f, 0.3f, 0.3f));
	groundInstance->Set("u_Roughness", 0.7f);

	// ------------------------------------------------------------------
	// Interactive sphere (ImGui-controlled) — placed to the right
	// ------------------------------------------------------------------
	m_InteractiveSphere = m_MainScene->AddEntity();
	auto& interactiveTransform = m_InteractiveSphere.GetComponent<Ember::TransformComponent>();
	interactiveTransform.Position = { (cols / 2) * spacing + 4.0f, 0.0f, 0.0f };
	interactiveTransform.Size     = { 1.5f, 1.5f, 1.5f };

	Ember::MeshComponent interactiveMeshComp = { mesh };
	m_InteractiveSphere.AttachComponent(interactiveMeshComp);

	Ember::MaterialComponent interactiveMatComp = { pbrMaterial };
	m_InteractiveSphere.AttachComponent(interactiveMatComp);

	m_InteractiveInstance = m_InteractiveSphere.GetComponent<Ember::MaterialComponent>().GetInstanced();
	m_InteractiveInstance->Set("u_Albedo",    Ember::Vector3f(m_Albedo[0], m_Albedo[1], m_Albedo[2]));
	m_InteractiveInstance->Set("u_Metallic",  m_Metallic);
	m_InteractiveInstance->Set("u_Roughness", m_Roughness);
	m_InteractiveInstance->Set("u_AO",        m_AO);
	m_InteractiveInstance->Set("u_Texture",   Ember::Renderer3D::GetWhiteTexture());

	// ------------------------------------------------------------------
	// Camera
	// ------------------------------------------------------------------
	m_CameraEntity = m_MainScene->AddEntity();
	auto& camTransform     = m_CameraEntity.GetComponent<Ember::TransformComponent>();
	camTransform.Position  = { 0.0f, 0.0f, 25.0f };

	Ember::Camera camera;
	camera.SetViewportSize(
		Ember::Application::Instance().GetWindow().GetWidth(),
		Ember::Application::Instance().GetWindow().GetHeight());
	camera.SetProjectionType(Ember::Camera::ProjectionType::Perspective);
	camera.SetPerspective(65.0f, 0.1f, 200.0f);

	Ember::CameraComponent cameraComponent(camera, true);
	m_CameraEntity.AttachComponent(cameraComponent);
	m_CameraEntity.AttachComponent<Ember::ScriptComponent>().Bind<Camera3DController>();

	// ------------------------------------------------------------------
	// Lighting — 4 point lights in a key / fill / rim arrangement
	//
	// Key light    – strong warm-white from upper-front-right
	// Fill light   – softer cool-white from upper-front-left (reduces harsh shadows)
	// Rim lights   – two behind the grid to create edge highlights
	//
	// Asymmetric placement ensures specular highlights shift across
	// roughness/metallic rows, clearly showing how PBR responds.
	// ------------------------------------------------------------------
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
		lt.Size     = { 0.3f, 0.3f, 0.3f };

		Ember::MeshComponent lightCubeMeshComp = { lightCubeMesh };
		lightEntity.AttachComponent(lightCubeMeshComp);

		Ember::MaterialComponent lightCubeMatComp = { pbrMaterial };
		lightEntity.AttachComponent(lightCubeMatComp);

		auto lightCubeInstance = lightEntity.GetComponent<Ember::MaterialComponent>().GetInstanced();
		lightCubeInstance->Set("u_Albedo",    Ember::Vector3f(1.0f, 1.0f, 1.0f));
		lightCubeInstance->Set("u_Roughness", 1.0f);
	}
}

void PBRTestLayer::OnDetach()
{
}

void PBRTestLayer::OnUpdate(Ember::TimeStep delta)
{
	m_MainScene->OnUpdate(delta);
}

void PBRTestLayer::OnImGuiRender(Ember::TimeStep delta)
{
	ImGui::Begin("PBR Material Editor");

	ImGui::Text("Interactive Sphere");
	ImGui::Separator();

	bool changed = false;

	changed |= ImGui::ColorEdit3("Albedo",    m_Albedo);
	changed |= ImGui::SliderFloat("Metallic",  &m_Metallic,  0.0f, 1.0f);
	changed |= ImGui::SliderFloat("Roughness", &m_Roughness, 0.05f, 1.0f);
	changed |= ImGui::SliderFloat("AO",        &m_AO,        0.0f, 1.0f);

	if (changed && m_InteractiveInstance)
	{
		m_InteractiveInstance->Set("u_Albedo",    Ember::Vector3f(m_Albedo[0], m_Albedo[1], m_Albedo[2]));
		m_InteractiveInstance->Set("u_Metallic",  m_Metallic);
		m_InteractiveInstance->Set("u_Roughness", m_Roughness);
		m_InteractiveInstance->Set("u_AO",        m_AO);
	}

	ImGui::Separator();
	ImGui::TextWrapped(
		"Grid: columns = roughness (0.05 -> 1.0), "
		"rows = metallic (0.0 -> 1.0)");

	ImGui::End();
}
