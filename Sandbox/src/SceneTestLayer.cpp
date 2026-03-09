#include "ebpch.h"
#include "SceneTestLayer.h"

SceneTestLayer::SceneTestLayer()
	: Layer("Scene Test Layer"), m_MainScene(Ember::SharedPtr<Ember::Scene>::Create("Scene1"))
{
}

SceneTestLayer::~SceneTestLayer()
{
}

void SceneTestLayer::OnAttach()
{
	m_Entity = m_MainScene->AddEntity();
	Ember::SpriteComponent spriteComp = { Ember::Vector4f(1.0f, 0.0f, 0.0f, 1.0f) };
	m_Entity->AttachComponent<Ember::SpriteComponent>(spriteComp);

	Ember::RigidBodyComponent rigidComp = { Ember::Vector3f(0.0f, 0.0f, 0.0f) };
	m_Entity->AttachComponent<Ember::RigidBodyComponent>(rigidComp);
}

void SceneTestLayer::OnDetatch()
{

}

void SceneTestLayer::OnUpdate(Ember::TimeStep delta)
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
	}

	m_MainScene->OnUpdate(delta);
}

void SceneTestLayer::OnImGuiRender(Ember::TimeStep delta)
{

}
