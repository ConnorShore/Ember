#include "ebpch.h"
#include "Scene.h"

#include "Ember/ECS/Component/Components.h"
#include "Ember/ECS/System/PhysicsSystem.h"
#include "Ember/ECS/System/Rendersystem.h"

namespace Ember {

	Scene::Scene(const std::string name)
		: m_Registry(ScopedPtr<Registry>::Create()), m_Name(name), m_Camera(-3.0f, 3.0f, -3.0f, 3.0f)
	{
		m_Registry->RegisterSystem(SharedPtr<PhysicsSystem>::Create());
		m_Registry->RegisterSystem(SharedPtr<RenderSystem>::Create());

		// Temporary camera to render objects
		Entity cameraEntity = m_Registry->CreateEntity();
		CameraComponent cameraComp(&m_Camera);
		m_Registry->AttachComponent(cameraEntity, cameraComp);
	}

	Scene::~Scene()
	{
	}

	void Scene::OnUpdate(TimeStep delta)
	{
		m_Registry->UpdateSystems(delta);
	}

	Ember::Entity Scene::AddEntity()
	{
		Entity entity = m_Registry->CreateEntity();
		TagComponent tag("New Entity");
		TransformComponent transform({ 0.0f, 0.0f, 0.0f });
		m_Registry->AttachComponent(entity, tag);
		m_Registry->AttachComponent(entity, transform);
		return entity;
	}

	void Scene::RemoveEntity(const Entity& entity)
	{
		m_Registry->DestroyEntity(entity);
	}

}