#include "ebpch.h"
#include "Test3DLayer.h"

class Camera3DController : public Ember::Behavior
{
public:
	void OnUpdate(Ember::TimeStep delta) override
	{
		float speed = 2.5f * delta;
		float rotSpeed = 1.5f * delta;

		if (Ember::Input::IsKeyPressed(Ember::KeyCode::Up))
			Transform().Position += Transform().GetForward() * speed;
		else if (Ember::Input::IsKeyPressed(Ember::KeyCode::Down))
			Transform().Position -= Transform().GetForward() * speed;

		if (Ember::Input::IsKeyPressed(Ember::KeyCode::Right))
			Transform().Rotation.y -= rotSpeed;
		else if (Ember::Input::IsKeyPressed(Ember::KeyCode::Left))
			Transform().Rotation.y += rotSpeed;
	}
};

Test3DLayer::Test3DLayer()
	: Layer("3D Test Layer"), m_MainScene(Ember::SharedPtr<Ember::Scene>::Create("Scene1"))
{

}

Test3DLayer::~Test3DLayer()
{

}

void Test3DLayer::OnAttach()
{
	auto mesh = Ember::PrimitiveGenerator::CreateSphere(1.0f, 32, 32);
	auto shader = RegisterShader("assets/shaders/test3D.glsl");
	auto pbrShader = RegisterShader("assets/shaders/pbr.glsl");
	auto texture = RegisterTexture("assets/textures/case.jpg");
	auto pbrMaterial = RegisterMaterial("pbrMaterial", pbrShader, {
		{ "u_Albedo", Ember::Vector3f(0.5f, 0.5f, 0.5f) },
		{ "u_Metallic", 0.0f },
		{ "u_Roughness", 0.5f },
		{ "u_AO", 1.0f },
		{ "u_Texture", Ember::Renderer3D::GetWhiteTexture() }
		});

	// Main Entity
	//m_Entity = m_MainScene->AddEntity();
	//auto& entityTransform = m_Entity.GetComponent<Ember::TransformComponent>();

	//Ember::MeshComponent meshComp = { mesh };
	//m_Entity.AttachComponent(meshComp);

	//Ember::MaterialComponent matComp = { pbrMaterial };
	//m_Entity.AttachComponent(matComp);

	//Ember::RigidBodyComponent rigidComp = { Ember::Vector3f(0.0f, 0.0f, 0.0f) };
	//m_Entity.AttachComponent(rigidComp);

	//Ember::ScriptComponent entityScript;
	//entityScript.OnUpdate = [](Ember::Entity e, Ember::TimeStep delta)
	//	{
	//		auto& rigidComp = e.GetComponent<Ember::RigidBodyComponent>();
	//		auto& transformComp = e.GetComponent<Ember::TransformComponent>();
	//		rigidComp.Velocity = Ember::Vector3f(0.0f, 0.0f, 0.0f);

	//		if (Ember::Input::IsKeyPressed(Ember::KeyCode::W))
	//		{
	//			rigidComp.Velocity = Ember::Vector3f(0.0f, 1.5f, 0.0f);
	//		}
	//		else if (Ember::Input::IsKeyPressed(Ember::KeyCode::S))
	//		{
	//			rigidComp.Velocity = Ember::Vector3f(0.0f, -1.5f, 0.0f);
	//		}
	//		if (Ember::Input::IsKeyPressed(Ember::KeyCode::D))
	//		{
	//			rigidComp.Velocity = Ember::Vector3f(1.5f, 0.0f, 0.0f);
	//		}
	//		else if (Ember::Input::IsKeyPressed(Ember::KeyCode::A))
	//		{
	//			rigidComp.Velocity = Ember::Vector3f(-1.5f, 0.0f, 0.0f);
	//		}

	//		if (Ember::Input::IsKeyPressed(Ember::KeyCode::E))
	//		{
	//			transformComp.Rotation.y += 1.5f * delta;
	//		}
	//		else if (Ember::Input::IsKeyPressed(Ember::KeyCode::Q))
	//		{
	//			transformComp.Rotation.y -= 1.5f * delta;
	//		}
	//	};
	//m_Entity.AttachComponent(entityScript);

	// Spheres - evenly spaced along X axis, each with a per-entity MaterialInstance
	// Roughness and metallic both ramp from 0.0 (left) to 1.0 (right)
	for (int i = 0; i < 5; i++) {
		float roughness = (float)i / 4.0f;
		float metallic  = 1.0f - ((float)i / 4.0f);

		auto sphere = m_MainScene->AddEntity();
		auto& transform = sphere.GetComponent<Ember::TransformComponent>();
		transform.Position = { -5.0f + i * 2.5f, 0.0f, 0.0f };

		Ember::MeshComponent meshComp = { mesh };
		sphere.AttachComponent(meshComp);

		Ember::MaterialComponent matComp = { pbrMaterial };
		sphere.AttachComponent(matComp);

		auto instance = sphere.GetComponent<Ember::MaterialComponent>().GetInstanced();
		instance->Set("u_Albedo",    Ember::Vector3f(0.5f, 0.5f, 0.5f));
		instance->Set("u_Metallic",  metallic);
		instance->Set("u_Roughness", roughness);
		instance->Set("u_AO",        1.0f);
		instance->Set("u_Texture",   Ember::Renderer3D::GetWhiteTexture());
	}


	// Camera Entity
	m_CameraEntity = m_MainScene->AddEntity();
	auto& transform = m_CameraEntity.GetComponent<Ember::TransformComponent>();
	transform.Position = { 0.0f, 0.0f, 10.0f };

	Ember::Camera camera;
	camera.SetViewportSize(Ember::Application::Instance().GetWindow().GetWidth(), Ember::Application::Instance().GetWindow().GetHeight());
	camera.SetProjectionType(Ember::Camera::ProjectionType::Perspective);
	camera.SetPerspective(65.0f, 0.1f, 120.0f);

	Ember::CameraComponent cameraComponent(camera, true);
	m_CameraEntity.AttachComponent(cameraComponent);
	m_CameraEntity.AttachComponent<Ember::ScriptComponent>().Bind<Camera3DController>();

	// Lights - 4 white lights at the corners of the scene (standard PBR test rig)
	// White color avoids biasing the perceived roughness/metallic response
	// Intensity 300 with inverse-square attenuation gives ~1.5 radiance at center spheres
	auto light1 = m_MainScene->AddEntity();
	Ember::PointLightComponent pl1 = { Ember::Vector3f(1.0f, 1.0f, 1.0f), 300.0f, 10.0f };
	light1.AttachComponent(pl1);
	Ember::TransformComponent t1 = { Ember::Vector3f(-8.0f, 8.0f, 8.0f) };  // front left
	light1.AttachComponent(t1);

	auto light2 = m_MainScene->AddEntity();
	Ember::PointLightComponent pl2 = { Ember::Vector3f(1.0f, 1.0f, 1.0f), 300.0f, 10.0f };
	light2.AttachComponent(pl2);
	Ember::TransformComponent t2 = { Ember::Vector3f(8.0f, 8.0f, 8.0f) };   // front right
	light2.AttachComponent(t2);

	auto light3 = m_MainScene->AddEntity();
	Ember::PointLightComponent pl3 = { Ember::Vector3f(1.0f, 1.0f, 1.0f), 300.0f, 10.0f };
	light3.AttachComponent(pl3);
	Ember::TransformComponent t3 = { Ember::Vector3f(8.0f, 8.0f, -8.0f) };  // back right
	light3.AttachComponent(t3);

	auto light4 = m_MainScene->AddEntity();
	Ember::PointLightComponent pl4 = { Ember::Vector3f(1.0f, 1.0f, 1.0f), 300.0f, 10.0f };
	light4.AttachComponent(pl4);
	Ember::TransformComponent t4 = { Ember::Vector3f(-8.0f, 8.0f, -8.0f) }; // back left
	light4.AttachComponent(t4);
}

void Test3DLayer::OnDetach()
{

}

void Test3DLayer::OnUpdate(Ember::TimeStep delta)
{
	m_MainScene->OnUpdate(delta);
}

void Test3DLayer::OnImGuiRender(Ember::TimeStep delta)
{

}
