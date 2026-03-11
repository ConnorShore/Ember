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
	RegisterMesh("assets/mesh/DefaultMesh.obj");
	RegisterShader("assets/shaders/test3D.glsl");
	RegisterTexture("assets/textures/Cube.png");

	// Main Entity
	m_Entity = m_MainScene->AddEntity();
	auto& entityTransform = m_Entity.GetComponent<Ember::TransformComponent>();
	entityTransform.Size = Ember::Vector3f(3.0f);

	Ember::MeshComponent meshComp = { GetMesh("DefaultMesh") };
	m_Entity.AttachComponent(meshComp);

	Ember::MaterialComponent matComp = { GetShader("test3D"), GetTexture("Cube") };
	m_Entity.AttachComponent(matComp);

	Ember::RigidBodyComponent rigidComp = { Ember::Vector3f(0.0f, 0.0f, 0.0f) };
	m_Entity.AttachComponent(rigidComp);

	Ember::ScriptComponent entityScript;
	entityScript.OnUpdate = [](Ember::Entity e, Ember::TimeStep delta)
		{
			auto& rigidComp = e.GetComponent<Ember::RigidBodyComponent>();
			auto& transformComp = e.GetComponent<Ember::TransformComponent>();
			rigidComp.Velocity = Ember::Vector3f(0.0f, 0.0f, 0.0f);

			if (Ember::Input::IsKeyPressed(Ember::KeyCode::W))
			{
				rigidComp.Velocity = Ember::Vector3f(0.0f, 1.5f, 0.0f);
			}
			else if (Ember::Input::IsKeyPressed(Ember::KeyCode::S))
			{
				rigidComp.Velocity = Ember::Vector3f(0.0f, -1.5f, 0.0f);
			}
			if (Ember::Input::IsKeyPressed(Ember::KeyCode::D))
			{
				rigidComp.Velocity = Ember::Vector3f(1.5f, 0.0f, 0.0f);
			}
			else if (Ember::Input::IsKeyPressed(Ember::KeyCode::A))
			{
				rigidComp.Velocity = Ember::Vector3f(-1.5f, 0.0f, 0.0f);
			}

			if (Ember::Input::IsKeyPressed(Ember::KeyCode::E))
			{
				transformComp.Rotation.y += 1.5f * delta;
			}
			else if (Ember::Input::IsKeyPressed(Ember::KeyCode::Q))
			{
				transformComp.Rotation.y -= 1.5f * delta;
			}
		};
	m_Entity.AttachComponent(entityScript);

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
