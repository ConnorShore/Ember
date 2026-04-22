#include "ebpch.h"
#include "Pool.h"

#include "Ember/Scene/Scene.h"
#include "Ember/Asset/Prefab.h"
#include "Ember/Core/Application.h"
#include "Ember/ECS/System/TransformSystem.h"
#include "Ember/ECS/System/PhysicsSystem.h"

#include <reactphysics3d/reactphysics3d.h>

namespace Ember {

	// Recursively syncs all physics body transforms in the hierarchy to match WorldTransform
	static void SyncPhysicsTransforms(EntityID entity, Scene* scene)
	{
		auto& registry = scene->GetRegistry();

		if (registry.ContainsComponent<RigidBodyComponent>(entity))
		{
			auto& rb = registry.GetComponent<RigidBodyComponent>(entity);
			if (rb.Body)
			{
				auto& transform = registry.GetComponent<TransformComponent>(entity);
				Vector3f worldPos, worldRot, worldScale;
				Math::DecomposeTransform(transform.WorldTransform, worldPos, worldRot, worldScale);
				Quaternion q = Math::ToQuaternion(worldRot);
				rb.Body->setTransform(rp3d::Transform(
					rp3d::Vector3(worldPos.x, worldPos.y, worldPos.z),
					rp3d::Quaternion(q.x, q.y, q.z, q.w)
				));
				rb.Body->setLinearVelocity(rp3d::Vector3(0.f, 0.f, 0.f));
				rb.Body->setAngularVelocity(rp3d::Vector3(0.f, 0.f, 0.f));
			}
		}

		auto& relationship = registry.GetComponent<RelationshipComponent>(entity);
		for (UUID childUUID : relationship.Children)
		{
			Entity child = scene->GetEntity(childUUID);
			if (child.GetEntityHandle() != Constants::Entities::InvalidEntityID)
				SyncPhysicsTransforms(child.GetEntityHandle(), scene);
		}
	}

	Pool::Pool(Scene* scene, const std::string& poolID, UUID prefabUUID, uint32_t initialSize)
		: Pool(scene, poolID, scene->GetAsset<Prefab>(prefabUUID), initialSize)
	{
	}

	Pool::Pool(Scene* scene, const std::string& poolID, const SharedPtr<Prefab>& prefab, uint32_t initialSize)
		: m_Id(poolID), m_SceneHandle(scene), m_Capacity(initialSize), m_PrefabUUID(prefab->GetUUID())
	{
		m_AvailableEntities = std::queue<EntityID>();

		for (size_t i = 0; i < initialSize; i++)
			m_AvailableEntities.push(CreatePooledEntity(prefab));

		EB_CORE_INFO("Created pool '{}' with {} entities", poolID, initialSize);
	}

	Entity Pool::Retrieve()
	{
		if (m_AvailableEntities.empty())
		{
#ifdef EB_DEBUG
			// Dynamically resize for debug builds to prevent frustrating crashes during development, 
			// but warn about it and suggest increasing the initial size for better performance
			uint32_t newSize = (uint32_t)(m_Capacity * 1.5f);
			EB_CORE_WARN("Pool '{}' has run out of available entities! Increasing size to {}", m_Id, newSize);

			Resize(newSize);
			return Retrieve();
#else
			EB_CORE_WARN("Pool '{}' has run out of available entities! Consider increasing the initial size.", m_Id);
			return Entity(Constants::Entities::InvalidEntityID, m_SceneHandle);
#endif
		}

		EntityID entity = m_AvailableEntities.front();
		m_AvailableEntities.pop();

		if (m_SceneHandle->GetRegistry().ContainsComponent<DisabledComponent>(entity))
			m_SceneHandle->GetRegistry().DetachComponent<DisabledComponent>(entity);

		// Update the world transforms for this entity and its children, then sync
		// all physics bodies to those transforms and clear any residual velocity.
		// This ensures the bullet/entity starts at the correct position with no
		// leftover momentum from its previous use.
		auto& systemManager = Application::Instance().GetSystemManager();
		systemManager.GetSystem<TransformSystem>()->UpdateTransformTree(entity, Matrix4f(1.0f), m_SceneHandle);
		SyncPhysicsTransforms(entity, m_SceneHandle);

		return Entity(entity, m_SceneHandle);
	}

	Entity Pool::Retrieve(const Vector3f& position)
	{
		if (m_AvailableEntities.empty())
		{
#ifdef EB_DEBUG
			uint32_t newSize = (uint32_t)(m_Capacity * 1.5f);
			EB_CORE_WARN("Pool '{}' has run out of available entities! Increasing size to {}", m_Id, newSize);

			Resize(newSize);
			return Retrieve(position);
#else
			EB_CORE_WARN("Pool '{}' has run out of available entities! Consider increasing the initial size.", m_Id);
			return Entity(Constants::Entities::InvalidEntityID, m_SceneHandle);
#endif
		}

		EntityID entity = m_AvailableEntities.front();
		m_AvailableEntities.pop();

		if (m_SceneHandle->GetRegistry().ContainsComponent<DisabledComponent>(entity))
			m_SceneHandle->GetRegistry().DetachComponent<DisabledComponent>(entity);

		// Set the spawn position before updating transforms and syncing physics,
		// so the body is placed correctly before any impulse is applied by the caller.
		m_SceneHandle->GetRegistry().GetComponent<TransformComponent>(entity).Position = position;

		auto& systemManager = Application::Instance().GetSystemManager();
		systemManager.GetSystem<TransformSystem>()->UpdateTransformTree(entity, Matrix4f(1.0f), m_SceneHandle);
		SyncPhysicsTransforms(entity, m_SceneHandle);

		return Entity(entity, m_SceneHandle);
	}

	void Pool::Return(EntityID entity)
	{
		Entity e(entity, m_SceneHandle);

		DisabledComponent dc;
		e.AttachComponent<DisabledComponent>(dc);

		m_AvailableEntities.push(entity);
	}

	void Pool::Clear()
	{
		for (size_t i = 0; i < m_AvailableEntities.size(); i++)
		{
			EntityID entity = m_AvailableEntities.front();
			m_AvailableEntities.pop();
			m_SceneHandle->RemoveEntity(Entity(entity, m_SceneHandle));
		}
	}

	EntityID Pool::CreatePooledEntity(const SharedPtr<Prefab>& prefab)
	{
		auto entity = m_SceneHandle->InstantiatePrefab(prefab, nullptr);

		DisabledComponent dc;
		entity.AttachComponent(dc);

		PoolComponent pc(m_Id);
		entity.AttachComponent(pc);

		return entity;
	}

#ifdef EB_DEBUG
	void Pool::Resize(uint32_t newSize)
	{
		auto prefab = m_SceneHandle->GetAsset<Prefab>(m_PrefabUUID);

		m_AvailableEntities = std::queue<EntityID>();
		for (size_t i = 0; i < newSize; i++)
			m_AvailableEntities.push(CreatePooledEntity(prefab));

		m_Capacity = newSize;
	}
#else
	void Pool::Resize(const SharedPtr<Prefab>& prefab, uint32_t newSize) {}
#endif

}