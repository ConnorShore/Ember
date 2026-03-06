#pragma once

#include "Types.h"
#include "Entity.h"

#include "Ember/Core/Core.h"

#include <vector>
#include <concepts>

namespace Ember {

	//////////////////////////////////////////////////////////////////////////
	// Component Memory Array Structs
	//////////////////////////////////////////////////////////////////////////

	struct ComponentMemoryArraysBase : public SharedResource
	{
		virtual ~ComponentMemoryArraysBase() = default;
		virtual void EntityDestroyed(Entity entity) = 0;
	};

	template<typename T>
	struct ComponentMemoryArray : public ComponentMemoryArraysBase
	{
		std::vector<EntityID> SparseEntityArray;
		std::vector<unsigned int> DenseEntityArray;
		std::vector<T> DenseComponentArray;

		ComponentMemoryArray()
		{
			SparseEntityArray.resize(EB_MAX_ENTITIES, EB_INVALID_ENTITY_ID);
		}

		void InsertComponent(Entity entity, T component)
		{
			// Add component to the dense component array and get its index
			unsigned int componentIndex = DenseComponentArray.size();

			// Map the component index to which entity its for
			DenseComponentArray.push_back(component);
			DenseEntityArray.push_back(componentIndex);

			// Add entity to spare array to point to the mapper array to eventually point to the component
			SparseEntityArray[entity] = componentIndex;
		}

		void RemoveComponent(Entity entity)
		{
			// Get the index of the component in the dense arrays
			unsigned int componentIndex = SparseEntityArray[entity];

			// Swap and pop the component //
			unsigned int lastComponentIndex = DenseComponentArray.size() - 1;
			unsigned int entityReplaceId = DenseEntityArray[lastComponentIndex];

			// Overwrite the component we want to remove with the last component in the dense array
			// to keep the array dense
			DenseComponentArray[componentIndex] = DenseComponentArray[lastComponentIndex];

			// Update the mapping for that last component's new location
			DenseEntityArray[componentIndex] = DenseComponentArray[lastComponentIndex];
			SparseEntityArray[entityReplaceId] = componentIndex;

			// Pop the dense arrays
			DenseComponentArray.pop_back();
			DenseEntityArray.pop_back();
		}

		virtual void EntityDestroyed(Entity entity) override
		{
			EB_CORE_ASSERT((entity < SparseEntityArray.size() && SparseEntityArray[entity] != EB_INVALID_ENTITY_ID),
				"Entity does not exist for component type!");

			RemoveComponent(entity);
		}
	};

	//////////////////////////////////////////////////////////////////////////
	// Component Manager
	//////////////////////////////////////////////////////////////////////////

	class ComponentManager
	{
	public:
		ComponentManager() = default;
		~ComponentManager() = default;

		template<typename T>
		inline void AddComponent(Entity entity, T component)
		{
			ComponentType type = GetComponentType<T>();

			// Add component type to components array if it doesn't exist yet
			if (type > m_ComponentArrays.size() - 1)
				m_ComponentArrays.resize(type + 1);

			if (!m_ComponentArrays[type])
				m_ComponentArrays[type] = SharedPtr<ComponentMemoryArray<T>>::Create();

			SharedPtr<ComponentMemoryArray<T>> memoryArrays = StaticPointerCast<ComponentMemoryArray<T>>(m_ComponentArrays[type]);
			memoryArrays->InsertComponent(entity, component);
		}

		template<typename T>
		inline void RemoveComponent(Entity entity)
		{
			ComponentType type = GetComponentType<T>();

			SharedPtr<ComponentMemoryArray<T>> memoryArrays = StaticPointerCast<ComponentMemoryArray<T>>(m_ComponentArrays[type]);
			memoryArrays->RemoveComponent(entity);
		}

		inline void EntityDestroyed(Entity entity)
		{
			for (auto compArray : m_ComponentArrays)
			{
				if (compArray != nullptr)
				{
					compArray->EntityDestroyed(entity);
				}
			}
		}

		template<typename T>
		inline ComponentType GetComponentType()
		{
			// Generates once per type and then is cached
			static const ComponentType typeId = GetNextComponentType();
			return typeId;
		}
	private:
		inline ComponentType GetNextComponentType()
		{
			static ComponentType nextId = 0;
			return nextId++;
		}

	private:
		std::vector<SharedPtr<ComponentMemoryArraysBase>> m_ComponentArrays;
	};

}