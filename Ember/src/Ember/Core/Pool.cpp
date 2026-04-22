#include "ebpch.h"
#include "Pool.h"

#include "Ember/Scene/Scene.h"
#include "Ember/Asset/Prefab.h"

namespace Ember {

	Pool::Pool(Scene* scene, const std::string& poolID, UUID prefabUUID, uint32_t initialSize)
		: Pool(scene, poolID, scene->GetAsset<Prefab>(prefabUUID), initialSize)
	{
	}

	Pool::Pool(Scene* scene, const std::string& poolID, const SharedPtr<Prefab>& prefab, uint32_t initialSize)
		: m_Id(poolID), m_SceneHandle(scene)
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
			// TOOD: Dynamically size up the pool if it runs out of entities instead of just returning an invalid id
			EB_CORE_WARN("Pool '{}' has run out of available entities! Consider increasing the initial size.", m_Id);
			return Entity(Constants::Entities::InvalidEntityID, m_SceneHandle);
		}


		EntityID entity = m_AvailableEntities.front();
		m_AvailableEntities.pop();

		if (m_SceneHandle->GetRegistry().ContainsComponent<DisabledComponent>(entity))
			m_SceneHandle->GetRegistry().DetachComponent<DisabledComponent>(entity);

		return Entity(entity, m_SceneHandle);
	}

	void Pool::Return(EntityID entity)
	{
		Entity e(entity, m_SceneHandle);

		DisabledComponent dc;
		e.AttachComponent<DisabledComponent>(dc);

		m_AvailableEntities.push(e);
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
}