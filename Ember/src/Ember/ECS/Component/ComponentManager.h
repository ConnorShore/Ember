#pragma once

#include "Types.h"
#include "Ember/Core/Core.h"

#include <vector>
#include <concepts>
#include <tuple>
#include <ranges>

namespace Ember {

	//////////////////////////////////////////////////////////////////////////
	// Component Memory Array Structs
	//////////////////////////////////////////////////////////////////////////

	struct ComponentMemoryArraysBase : public SharedResource
	{
		virtual ~ComponentMemoryArraysBase() = default;
		virtual void EntityDestroyed(EntityID entity) = 0;
	};

	template<typename T>
	struct ComponentMemoryArray : public ComponentMemoryArraysBase
	{
		std::vector<EntityID> SparseEntityArray;
		std::vector<unsigned int> DenseEntityArray;
		std::vector<T> DenseComponentArray;

		ComponentMemoryArray()
		{
			SparseEntityArray.resize(Constants::Entities::MaxEntities, Constants::Entities::InvalidComponentID);
		}

		T& InsertComponent(EntityID entity, T component)
		{
			// Add component to the dense component array and get its index
			unsigned int componentIndex = DenseComponentArray.size();

			// Map the component index to which entity its for
			DenseComponentArray.push_back(component);
			DenseEntityArray.push_back(entity);

			// Add entity to spare array to point to the mapper array to eventually point to the component
			SparseEntityArray[entity] = componentIndex;
			return DenseComponentArray[componentIndex];
		}

		T& GetComponent(EntityID entity)
		{
			EB_CORE_ASSERT(SparseEntityArray[entity] != Constants::Entities::InvalidComponentID, "Attempting to retrieve a non-existent component!");
			return DenseComponentArray[SparseEntityArray[entity]];
		}

		void RemoveComponent(EntityID entity)
		{
			// Get the index of the component in the dense arrays
			unsigned int componentIndex = SparseEntityArray[entity];

			if (componentIndex == Constants::Entities::InvalidComponentID)
			{
				EB_CORE_WARN("Entity {} does not contain component type!", entity);
				return;
			}

			// Swap and pop the component //
			unsigned int lastComponentIndex = DenseComponentArray.size() - 1;
			unsigned int entityReplaceId = DenseEntityArray[lastComponentIndex];

			// Overwrite the component we want to remove with the last component in the dense array
			// to keep the array dense
			DenseComponentArray[componentIndex] = DenseComponentArray[lastComponentIndex];

			// Update the mapping for that last component's new location
			DenseEntityArray[componentIndex] = entityReplaceId;
			SparseEntityArray[entityReplaceId] = componentIndex;

			// Pop the dense arrays
			SparseEntityArray[entity] = Constants::Entities::InvalidComponentID;
			DenseComponentArray.pop_back();
			DenseEntityArray.pop_back();
		}

		virtual void EntityDestroyed(EntityID entity) override
		{
			if (entity < SparseEntityArray.size() && SparseEntityArray[entity] != Constants::Entities::InvalidComponentID)
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
		~ComponentManager() { m_ComponentArrays.clear(); }

		template<typename T>
		inline T& AttachComponent(EntityID entity, T component)
		{
			ComponentType type = GetComponentType<T>();

			// Add component type to components array if it doesn't exist yet
			if (m_ComponentArrays.empty() || type > m_ComponentArrays.size() - 1)
				m_ComponentArrays.resize(type + 1);

			if (m_ComponentArrays[type] == nullptr)
				m_ComponentArrays[type] = SharedPtr<ComponentMemoryArray<T>>::Create();

			SharedPtr<ComponentMemoryArray<T>> memoryArrays = StaticPointerCast<ComponentMemoryArray<T>>(m_ComponentArrays[type]);
			return memoryArrays->InsertComponent(entity, component);
		}

		template<typename T>
		inline void DetachComponent(EntityID entity)
		{
			ComponentType type = GetComponentType<T>();

			SharedPtr<ComponentMemoryArray<T>> memoryArrays = StaticPointerCast<ComponentMemoryArray<T>>(m_ComponentArrays[type]);
			memoryArrays->RemoveComponent(entity);
		}

		template<typename T>
		inline ComponentType GetComponentType()
		{
			// Generates once per type and then is cached
			static const ComponentType typeId = GetNextComponentType();
			return typeId;
		}

		template<typename T>
		inline T& GetComponent(EntityID entity)
		{
			ComponentType type = GetComponentType<T>();

			SharedPtr<ComponentMemoryArray<T>> memoryArrays = StaticPointerCast<ComponentMemoryArray<T>>(m_ComponentArrays[type]);
			return memoryArrays->GetComponent(entity);
		}

		template<typename T>
		const std::vector<EntityID>& GetActiveEntities()
		{
			ComponentType type = GetComponentType<T>();
			if (type >= m_ComponentArrays.size() || m_ComponentArrays[type] == nullptr)
			{
				static std::vector<EntityID> empty;
				return empty;
			}

			SharedPtr<ComponentMemoryArray<T>> memoryArrays = StaticPointerCast<ComponentMemoryArray<T>>(m_ComponentArrays[type]);
			return memoryArrays->DenseEntityArray;
		}


		inline void EntityDestroyed(EntityID entity)
		{
			for (auto compArray : m_ComponentArrays)
			{
				if (compArray != nullptr)
				{
					compArray->EntityDestroyed(entity);
				}
			}
		}

		inline const std::vector<SharedPtr<ComponentMemoryArraysBase>>& GetComponentArrays() { return m_ComponentArrays; }

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