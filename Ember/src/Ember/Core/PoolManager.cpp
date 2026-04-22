#include "ebpch.h"
#include "PoolManager.h"

#include "Ember/Scene/Scene.h"

namespace Ember {

	void PoolManager::CreatePool(Scene* scene, const std::string& poolID, UUID prefabUUID, uint32_t initialSize)
	{
		m_Pools[poolID] = ScopedPtr<Pool>::Create(scene, poolID, prefabUUID, initialSize);
	}

	void PoolManager::CreatePool(Scene* scene, const std::string& poolID, const SharedPtr<Prefab>& prefab, uint32_t initialSize)
	{
		m_Pools[poolID] = ScopedPtr<Pool>::Create(scene, poolID, prefab, initialSize);
	}

	void PoolManager::DestroyPools()
	{
		for (auto& [poolID, pool] : m_Pools)
		{
			pool->Clear();
		}

		m_Pools.clear();
		EB_CORE_INFO("Destroyed pools");
	}

	Entity PoolManager::RetrieveFromPool(Scene* scene, const std::string& poolID)
	{
		if (m_Pools.find(poolID) == m_Pools.end())
		{
			EB_CORE_ERROR("Pool '{}' does not exist!", poolID);
			return Entity();
		}

		return m_Pools[poolID]->Retrieve();
	}

	Entity PoolManager::RetrieveFromPool(Scene* scene, const std::string& poolID, const Vector3f& position)
	{
		if (m_Pools.find(poolID) == m_Pools.end())
		{
			EB_CORE_ERROR("Pool '{}' does not exist!", poolID);
			return Entity();
		}

		return m_Pools[poolID]->Retrieve(position);
	}

	void PoolManager::ReturnToPool(EntityID entity, const std::string& poolID)
	{
		if (m_Pools.find(poolID) == m_Pools.end())
		{
			EB_CORE_ERROR("Pool '{}' does not exist!", poolID);
			return;
		}
		m_Pools[poolID]->Return(entity);
	}

}