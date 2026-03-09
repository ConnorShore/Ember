#include "ebpch.h"
#include "Scene.h"

#include "Ember/ECS/Component/Components.h"
#include "Ember/ECS/System/PhysicsSystem.h"
#include "Ember/ECS/System/Rendersystem.h"

namespace Ember {

	Scene::Scene(const std::string& name)
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

	SharedPtr<SceneEntity> Scene::AddEntity()
	{
		std::string name = "Entity " + m_SceneEntities.size();
		return SharedPtr<SceneEntity>::Create(this, name);
	}

	SharedPtr<SceneEntity> Scene::GetEntity(const std::string& tag)
	{
		if (m_SceneEntities.find(tag) == m_SceneEntities.end())
		{
			EB_CORE_ASSERT(false, "Scene does not contain entity with tag!");
			return nullptr;
		}

		return m_SceneEntities[tag];
	}

	void Scene::RemoveEntity(const SharedPtr<SceneEntity>& entity)
	{
		auto it = m_SceneEntities.find(entity->GetName());
		EB_CORE_ASSERT(it != m_SceneEntities.end(), "Scene does not contain entity!");
		m_Registry->DestroyEntity(entity->GetEntityHandle());
		m_SceneEntities.erase(it);
	}

}