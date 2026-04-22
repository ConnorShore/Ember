#pragma once

#include "Pool.h"

#include <unordered_map>

namespace Ember {

	class Entity;
	class Scene;

	class PoolManager
	{
	public:
		PoolManager() = default;
		~PoolManager() = default;

		void CreatePool(Scene* scene, const std::string& poolID, UUID prefabUUID, uint32_t initialSize);
		void CreatePool(Scene* scene, const std::string& poolID, const SharedPtr<Prefab>& prefab, uint32_t initialSize);
		void DestroyPools();

		Entity RetrieveFromPool(Scene* scene, const std::string& poolID);
		void ReturnToPool(EntityID entity, const std::string& poolID);

	private:
		std::unordered_map<std::string, ScopedPtr<Pool>> m_Pools;
	};

}