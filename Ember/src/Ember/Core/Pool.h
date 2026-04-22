#pragma once

#include "Ember/Core/Core.h"
#include "Ember/ECS/Types.h"

#include "Ember/Math/Math.h"
#include "Ember/Asset/UUID.h"

#include <string>
#include <queue>

namespace Ember
{
	class Scene;
	class Prefab;
	class Entity;

	// Pool ideas:
	//  - Pool Manager will manage multiple pools, and will be responsible for allocating and deallocating memory for the pools
	//  - Entity is marked to be in a pool with PoolComponent (which will have a single member variable, pool id)

	class Pool
	{
	public:
		Pool(Scene* scene, const std::string& poolID, UUID prefabUUID, uint32_t initialSize);
		Pool(Scene* scene, const std::string& poolID, const SharedPtr<Prefab>& prefab, uint32_t initialSize);
		~Pool() = default;

		Entity Retrieve();
		Entity Retrieve(const Vector3f& position);
		void Return(EntityID entity);

		// TODO: This may not be necessary since the scene gets destroyed anyway
		void Clear();

	private:
		EntityID CreatePooledEntity(const SharedPtr<Prefab>& prefab);

	private:
		std::string m_Id;
		Scene* m_SceneHandle;
		std::queue<EntityID> m_AvailableEntities;
	};

}