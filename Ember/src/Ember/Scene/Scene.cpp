#include "ebpch.h"
#include "Scene.h"

#include "Ember/ECS/Component/Components.h"
#include "Ember/ECS/System/ScriptSystem.h"
#include "Ember/ECS/System/PhysicsSystem.h"
#include "Ember/ECS/System/Rendersystem.h"
#include "Ember/ECS/System/TransformSystem.h"

namespace Ember {

	Scene::Scene(const std::string& name)
		: m_Registry(ScopedPtr<Registry>::Create()), m_Name(name)
	{
		m_Registry->RegisterSystem(SharedPtr<ScriptSystem>::Create(this));
		m_Registry->RegisterSystem(SharedPtr<PhysicsSystem>::Create());
		m_Registry->RegisterSystem(SharedPtr<TransformSystem>::Create());
		m_Registry->RegisterSystem(SharedPtr<RenderSystem>::Create());
	}

	Scene::~Scene()
	{
	}

	void Scene::OnUpdateRuntime(TimeStep delta)
	{
		m_Registry->GetSystem<ScriptSystem>()->OnUpdate(delta, m_Registry.Ptr());
		m_Registry->GetSystem<PhysicsSystem>()->OnUpdate(delta, m_Registry.Ptr());
		m_Registry->GetSystem<TransformSystem>()->OnUpdate(delta, m_Registry.Ptr());
		m_Registry->GetSystem<RenderSystem>()->OnUpdate(delta, m_Registry.Ptr());
	}

	void Scene::OnUpdateEdit(TimeStep delta, EditorCamera& camera)
	{
		m_Registry->GetSystem<TransformSystem>()->OnUpdate(delta, m_Registry.Ptr());
		m_Registry->GetSystem<RenderSystem>()->OnUpdate(delta, m_Registry.Ptr(), camera, Math::Inverse(camera.GetViewMatrix()));
	}

	void Scene::OnEvent(Event& event)
	{
		EB_CREATE_DISPATCHER(event);
		EB_DISPATCH_EVENT(WindowResizeEvent, OnWindowResize);
	}

	void Scene::OnViewportResize(unsigned int width, unsigned int height)
	{
		auto view = m_Registry->Query<CameraComponent>();
		for (auto entity : view)
		{
			auto& camera = m_Registry->GetComponent<CameraComponent>(entity);
				camera.Camera.SetViewportSize(width, height);
		}

		// Notify the RenderSystem of the viewport resize so it can adjust framebuffer sizes accordingly
		auto renderSystem = m_Registry->GetSystem<RenderSystem>();
		if (renderSystem)
		{
			renderSystem->OnViewportResize(width, height);
		}
	}

	Entity Scene::AddEntity()
	{
		std::string name = std::format("Entity {}", m_SceneEntities.size());
		Entity entity(name, this);
		m_SceneEntities[name] = entity;
		return entity;
	}

	Entity Scene::AddEntity(const std::string& name)
	{
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

	Entity Scene::InstantiateModel(const SharedPtr<Model>& model, const std::string& name /*= ""*/)
	{
		Entity rootEntity = AddEntity(name.empty() ? model->GetName() : name);
		ProcessModelNode(rootEntity, model->GetRootNode(), model);
		return rootEntity;
	}

	Entity Scene::GetEntityAtPixel(unsigned int x, unsigned int y)
	{
		auto renderSystem = m_Registry->GetSystem<RenderSystem>();
		EntityID id = renderSystem->GetEntityIDAtPixel(x, y);
		return id != Constants::Entities::InvalidEntityID ? Entity(id, this) : Entity();
	}

	bool Scene::OnWindowResize(const WindowResizeEvent& event)
	{
		OnViewportResize(event.GetWidth(), event.GetHeight());
		return false;
	}

	void Scene::ProcessModelNode(Entity currentEntity, const ModelNode& node, const SharedPtr<Model>& model)
	{
		auto& transform = currentEntity.GetComponent<TransformComponent>();
		Math::DecomposeTransform(node.LocalTransform, transform.Position, transform.Rotation, transform.Scale);

		// 2. Handle Meshes (Respecting the 1-Component Rule)
		if (node.Meshes.size() == 1)
		{
			// Safe to attach directly!
			MeshComponent mc{ node.Meshes[0].MeshAsset };
			currentEntity.AttachComponent<MeshComponent>(mc);

			// Attach material
			MaterialComponent matComp{ model->GetAllMaterials()[node.Meshes[0].MaterialIndex] };
			currentEntity.AttachComponent<MaterialComponent>(matComp);
		}
		else if (node.Meshes.size() > 1)
		{
			// We must spawn sub-entities to hold the extra meshes
			for (size_t i = 0; i < node.Meshes.size(); i++)
			{
				Entity meshPartEntity = AddEntity(node.Name + "_Part" + std::to_string(i));

				// Link the relationship!
				auto& partRc = meshPartEntity.GetComponent<RelationshipComponent>();
				partRc.ParentHandle = currentEntity.GetEntityHandle();
				currentEntity.GetComponent<RelationshipComponent>().Children.push_back(meshPartEntity.GetEntityHandle());

				// Attach the mesh
				MeshComponent mc{ node.Meshes[i].MeshAsset };
				meshPartEntity.AttachComponent<MeshComponent>(mc);

				// Attach material
				MaterialComponent matComp{ model->GetAllMaterials()[node.Meshes[i].MaterialIndex] };
				currentEntity.AttachComponent<MaterialComponent>(matComp);
			}
		}

		// 3. Recursively process all child branches
		for (const auto& childNode : node.ChildNodes)
		{
			// Create the child entity
			Entity childEntity = AddEntity(childNode.Name);

			// Link the relationship to the current entity
			auto& childRc = childEntity.GetComponent<RelationshipComponent>();
			childRc.ParentHandle = currentEntity.GetEntityHandle();
			currentEntity.GetComponent<RelationshipComponent>().Children.push_back(childEntity.GetEntityHandle());

			// Recurse deeper into the tree
			ProcessModelNode(childEntity, childNode, model);
		}
	}

}