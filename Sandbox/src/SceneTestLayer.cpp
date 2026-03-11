#include "ebpch.h"
#include "SceneTestLayer.h"

#include <random>

//////////////////////////////////////////////////////////////////////////
// Camera Controller
//////////////////////////////////////////////////////////////////////////

class CameraController : public Ember::Behavior
{
public:
	void OnUpdate(Ember::TimeStep delta) override
	{
		float speed = 2.5f * delta;
		if (Ember::Input::IsKeyPressed(Ember::KeyCode::Up))
		{
			Transform().Position += Ember::Vector3f(0.0f, 0.0f, -speed);
		}
		else if (Ember::Input::IsKeyPressed(Ember::KeyCode::Down))
		{
			Transform().Position += Ember::Vector3f(0.0f, 0.0f, speed);
		}
		if (Ember::Input::IsKeyPressed(Ember::KeyCode::Right))
		{
			Transform().Position += Ember::Vector3f(speed, 0.0f, 0.0f);
		}
		else if (Ember::Input::IsKeyPressed(Ember::KeyCode::Left))
		{
			Transform().Position += Ember::Vector3f(-speed, 0.0f, 0.0f);
		}
	}
};

//////////////////////////////////////////////////////////////////////////
// Scene Test Layer
//////////////////////////////////////////////////////////////////////////

SceneTestLayer::SceneTestLayer()
	: Layer("Scene Test Layer"), m_MainScene(Ember::SharedPtr<Ember::Scene>::Create("Scene1"))
{
}

SceneTestLayer::~SceneTestLayer()
{
}

void SceneTestLayer::OnAttach()
{
	// Entities
	std::mt19937 rng(std::random_device{}());
	std::uniform_real_distribution<float> colorDist(0.0f, 1.0f);
	std::uniform_real_distribution<float> posDist(-20.0f, 20.0f);

	m_SpriteEntities.reserve(250);
	for (int i = 0; i < 250; i++)
	{
		auto entity = m_MainScene->AddEntity();

		Ember::SpriteComponent spriteComp = { Ember::Vector4f(colorDist(rng), colorDist(rng), colorDist(rng), 1.0f) };
		entity.AttachComponent<Ember::SpriteComponent>(spriteComp);

		auto& transform = entity.GetComponent<Ember::TransformComponent>();
		transform.Position = Ember::Vector3f(posDist(rng), posDist(rng), 0.0f);

		m_SpriteEntities.push_back(entity);
	}

	RegisterTexture("assets/textures/image.png");

	// Main Entity
	m_Entity = m_MainScene->AddEntity();
	auto& entityTransform = m_Entity.GetComponent<Ember::TransformComponent>();
	entityTransform.Size = Ember::Vector3f(3.0f, 3.0f, 1.0f);

	Ember::SpriteComponent spriteComp = { GetTexture("image")};
	m_Entity.AttachComponent(spriteComp);

	Ember::RigidBodyComponent rigidComp = { Ember::Vector3f(0.0f, 0.0f, 0.0f) };
	m_Entity.AttachComponent(rigidComp);

	Ember::ScriptComponent entityScript;
	entityScript.OnUpdate = [](Ember::SceneEntity e, Ember::TimeStep delta)
		{
			auto& rigidComp = e.GetComponent<Ember::RigidBodyComponent>();
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
		};
	m_Entity.AttachComponent(entityScript);

	// Camera Entity
	m_CameraEntity = m_MainScene->AddEntity();
	auto& transform = m_CameraEntity.GetComponent<Ember::TransformComponent>();
	transform.Position = { 0.0f, 0.0f, 10.0f };

	Ember::Camera camera;
	camera.SetViewportSize(Ember::Application::Instance().GetWindow().GetWidth(), Ember::Application::Instance().GetWindow().GetHeight());
	camera.SetProjectionType(Ember::Camera::ProjectionType::Perspective);
	//camera.SetOrthographic(10.0f, -1.0f, 1.0f);
	camera.SetPerspective(60.0f, 0.1f, 120.0f);

	Ember::CameraComponent cameraComponent(camera, true);
	m_CameraEntity.AttachComponent(cameraComponent);
	m_CameraEntity.AttachComponent<Ember::ScriptComponent>().Bind<CameraController>();
}

void SceneTestLayer::OnDetach()
{

}

void SceneTestLayer::OnUpdate(Ember::TimeStep delta)
{
	m_MainScene->OnUpdate(delta);
}

void SceneTestLayer::OnImGuiRender(Ember::TimeStep delta)
{

}
