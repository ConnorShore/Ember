#include "ebpch.h"
#include "Scene.h"

#include "Ember/ECS/Component/Components.h"
#include "Ember/ECS/System/ScriptSystem.h"
#include "Ember/ECS/System/PhysicsSystem.h"
#include "Ember/ECS/System/Rendersystem.h"

namespace Ember {

	Scene::Scene(const std::string& name)
		: m_Registry(ScopedPtr<Registry>::Create()), m_Name(name)
	{
		m_Registry->RegisterSystem(SharedPtr<ScriptSystem>::Create(this));
		m_Registry->RegisterSystem(SharedPtr<PhysicsSystem>::Create());
		m_Registry->RegisterSystem(SharedPtr<RenderSystem>::Create());
	}

	Scene::~Scene()
	{
	}

	void Scene::OnUpdate(TimeStep delta)
	{
		m_Registry->UpdateSystems(delta);
	}

	void Scene::OnEvent(Event& event)
	{
		EB_CREATE_DISPATCHER(event);
		EB_DISPATCH_EVENT(WindowResizeEvent, OnWindowResize);
	}

	Entity Scene::AddEntity()
	{
		std::string name = std::format("Entity {}", m_SceneEntities.size());
		Entity entity(name, this);
		m_SceneEntities[name] = entity;
		return entity;
	}

	Entity Scene::GetEntity(const std::string& tag)
	{
		if (m_SceneEntities.find(tag) == m_SceneEntities.end())
		{
			EB_CORE_ASSERT(false, "Scene does not contain entity with tag!");
			return {};
		}

		return { m_SceneEntities[tag], this };
	}

	void Scene::RemoveEntity(const Entity& entity)
	{
		EB_CORE_ASSERT(m_SceneEntities.contains(entity.GetName()), "Scene does not contain entity!");
		m_SceneEntities.erase(entity.GetName());
		m_Registry->DestroyEntity(entity.GetEntityHandle());
	}

	bool Scene::OnWindowResize(const WindowResizeEvent& event)
	{
		auto view = m_Registry->Query<CameraComponent>();
		for (auto entity : view)
		{
			auto camera = m_Registry->GetComponent<CameraComponent>(entity);
			camera.Camera.SetViewportSize(event.GetWidth(), event.GetHeight());
		}
		return false;
	}

}