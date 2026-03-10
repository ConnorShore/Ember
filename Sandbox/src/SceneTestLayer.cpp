#include "ebpch.h"
#include "SceneTestLayer.h"

#include <random>

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
	std::uniform_real_distribution<float> posDist(-9.5f, 9.5f);

	m_SpriteEntities.reserve(250);
	for (int i = 0; i < 250; i++)
	{
		auto entity = m_MainScene->AddEntity();

		Ember::SpriteComponent spriteComp = { Ember::Vector4f(colorDist(rng), colorDist(rng), colorDist(rng), 1.0f) };
		entity->AttachComponent<Ember::SpriteComponent>(spriteComp);

		auto& transform = entity->GetComponent<Ember::TransformComponent>();
		transform.Position = Ember::Vector3f(posDist(rng), posDist(rng), 0.0f);

		m_SpriteEntities.push_back(entity);
	}

	RegisterTexture("assets/textures/image.png");

	// Main Entity

	m_Entity = m_MainScene->AddEntity();
	auto& entityTransform = m_Entity->GetComponent<Ember::TransformComponent>();
	entityTransform.Size = Ember::Vector3f(3.0f, 3.0f, 1.0f);
	Ember::SpriteComponent spriteComp = { GetTexture("image")};
	m_Entity->AttachComponent<Ember::SpriteComponent>(spriteComp);

	Ember::RigidBodyComponent rigidComp = { Ember::Vector3f(0.0f, 0.0f, 0.0f) };
	m_Entity->AttachComponent<Ember::RigidBodyComponent>(rigidComp);

	// Make a camera follow the player
	Ember::CameraComponent cameraComp(Ember::Math::Orthographic(-10.0f, 10.0f, -10.0f, 10.0f), true);
	m_Entity->AttachComponent(cameraComp);

	// Default Camera
	//auto cameraEntity = m_MainScene->AddEntity();
	//Ember::CameraComponent cameraComp(Ember::Math::Orthographic(-10.0f, 10.0f, -10.0f, 10.0f), true);
	//cameraEntity->AttachComponent(cameraComp);
}

void SceneTestLayer::OnDetach()
{

}

void SceneTestLayer::OnUpdate(Ember::TimeStep delta)
{
	static bool entityAlive = true;
	if (entityAlive)
	{
		auto& rigidComp = m_Entity->GetComponent<Ember::RigidBodyComponent>();
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

		if (Ember::Input::IsKeyPressed(Ember::KeyCode::Delete))
		{
			m_MainScene->RemoveEntity(m_Entity);
			entityAlive = false;
		}
	}

	m_MainScene->OnUpdate(delta);
}

void SceneTestLayer::OnImGuiRender(Ember::TimeStep delta)
{

}
