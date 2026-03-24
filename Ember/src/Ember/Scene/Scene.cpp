#include "ebpch.h"
#include "Scene.h"

#include "Ember/ECS/Component/Components.h"
#include "Ember/ECS/System/ScriptSystem.h"
#include "Ember/ECS/System/PhysicsSystem.h"
#include "Ember/ECS/System/Rendersystem.h"
#include "Ember/ECS/System/TransformSystem.h"
#include "Ember/Core/Application.h"

namespace Ember {

	namespace Utils {

		template<typename... Component>
		static void CopyComponents(Entity src, Entity dst)
		{
			([&]()
				{
					if (src.ContainsComponent<Component>())
					{
						dst.AttachComponent<Component>(src.GetComponent<Component>());
					}
				}(), ...);
		}
	}

	Scene::Scene(const std::string& name)
		: m_Registry(ScopedPtr<Registry>::Create()), m_Name(name)
	{
		m_Registry->RegisterSystem(SharedPtr<ScriptSystem>::Create(this));
		m_Registry->RegisterSystem(SharedPtr<PhysicsSystem>::Create());
		m_Registry->RegisterSystem(SharedPtr<TransformSystem>::Create(this));
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

	Entity Scene::AddEntity(const std::string& name)
	{
		return AddEntity(UUID(), name);
	}

	Entity Scene::AddEntity(UUID uuid, const std::string& name)
	{
		EntityID handle = m_Registry->CreateEntity();
		Entity entity = { handle, this };

		auto& id = entity.AttachComponent<IDComponent>();
		id.ID = uuid;

		auto& tag = entity.AttachComponent<TagComponent>();
		tag.Tag = name.empty() ? "Entity" : name;

		entity.AttachComponent<TransformComponent>();
		entity.AttachComponent<RelationshipComponent>();

		m_EntityUUIDMap[uuid] = handle;

		return entity;
	}

	Entity Scene::GetEntity(UUID uuid)
	{
		if (m_EntityUUIDMap.find(uuid) != m_EntityUUIDMap.end())
			return { m_EntityUUIDMap.at(uuid), this };

		return Entity();
	}

	Entity Scene::DuplicateEntity(Entity entity)
	{
		return DuplicateEntityRecursive(entity, entity.GetComponent<RelationshipComponent>().ParentHandle, true);
	}

	Entity Scene::DuplicateEntityRecursive(Entity entity, UUID newParentId, bool isRoot)
	{
		std::string name = entity.GetName();
		Entity newEntity = AddEntity(name + " (Copy)");

		Utils::CopyComponents<
			TransformComponent,
			MeshComponent,
			MaterialComponent,
			SpriteComponent,
			CameraComponent,
			ScriptComponent,
			RigidBodyComponent,
			DirectionalLightComponent,
			SpotLightComponent,
			PointLightComponent
		>(entity, newEntity);

		if (entity.ContainsComponent<RelationshipComponent>())
		{
			auto oldRels = entity.GetComponent<RelationshipComponent>();
			RelationshipComponent newRels;

			if (isRoot)
			{
				newRels.ParentHandle = oldRels.ParentHandle;

				if (newParentId != Constants::Entities::InvalidEntityUUID)
				{
					Entity newParent = GetEntity(newParentId);
					if (newParent)
						newParent.GetComponent<RelationshipComponent>().Children.push_back(newEntity.GetUUID());
				}
			}
			else
			{
				newRels.ParentHandle = newParentId;
			}

			for (UUID childUUID : oldRels.Children)
			{
				Entity childEntity = GetEntity(childUUID);
				if (childEntity != Constants::Entities::InvalidEntityID)
				{
					Entity duplicatedChild = DuplicateEntityRecursive(childEntity, newEntity.GetUUID(), false);
					newRels.Children.push_back(duplicatedChild.GetUUID());
				}
			}

			newEntity.AttachComponent(newRels);
		}

		return newEntity;
	}

	std::vector<Entity> Scene::GetAllEntities() const
	{
		std::vector<Entity> entities;
		entities.reserve(m_EntityUUIDMap.size());

		for (const auto& [uuid, id] : m_EntityUUIDMap)
			entities.emplace_back(id, const_cast<Scene*>(this));

		return entities;
	}

	void Scene::RemoveEntity(Entity entity)
	{
		EB_CORE_ASSERT(m_EntityUUIDMap.find(entity.GetUUID()) != m_EntityUUIDMap.end(), "Scene does not contain entity!");

		// Remove children first
		for (auto child : entity.GetAllChildren())
			RemoveEntity(child);

		// Remove from ECS and our Map
		UUID entityUUID = entity.GetUUID();
		m_Registry->DestroyEntity(entity.GetEntityHandle());
		m_EntityUUIDMap.erase(entityUUID);
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
				partRc.ParentHandle = currentEntity.GetUUID();
				currentEntity.GetComponent<RelationshipComponent>().Children.push_back(meshPartEntity.GetUUID());

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
			childRc.ParentHandle = currentEntity.GetUUID();
			currentEntity.GetComponent<RelationshipComponent>().Children.push_back(childEntity.GetUUID());

			// Recurse deeper into the tree
			ProcessModelNode(childEntity, childNode, model);
		}
	}

}