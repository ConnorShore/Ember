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
		virtual void RemoveComponent(EntityID entity) = 0;
		virtual void* GetComponentDataErased(EntityID entity) = 0;
	};

	// Sparse-set storage for a single component type
	// SparseEntityArray: entity ID -> index into dense arrays (or InvalidComponentID if absent)
	// DenseEntityArray:  packed entity IDs that have this component
	// DenseComponentArray: packed component data, parallel to DenseEntityArray
	template<typename T>
	struct ComponentMemoryArray : public ComponentMemoryArraysBase
	{
		std::vector<EntityID> SparseEntityArray;
		std::vector<uint32_t> DenseEntityArray;
		std::vector<T> DenseComponentArray;

		ComponentMemoryArray()
		{
			SparseEntityArray.resize(Constants::Entities::MaxEntities, Constants::Entities::InvalidComponentID);
		}

		T& InsertComponent(EntityID entity, T component)
		{
			// If the entity already has this component, update it in place
			if (SparseEntityArray[entity] != Constants::Entities::InvalidComponentID)
			{
				DenseComponentArray[SparseEntityArray[entity]] = component;
				return DenseComponentArray[SparseEntityArray[entity]];
			}

			// Add component to the dense component array and get its index
			uint32_t componentIndex = static_cast<uint32_t>(DenseComponentArray.size());

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

		virtual void* GetComponentDataErased(EntityID entity) override
		{
			if (SparseEntityArray[entity] == Constants::Entities::InvalidComponentID)
			{
				EB_CORE_WARN("Entity {} does not contain component type!", entity);
				return nullptr;
			}
			return &DenseComponentArray[SparseEntityArray[entity]];
		}

		virtual void RemoveComponent(EntityID entity) override
		{
			uint32_t componentIndex = SparseEntityArray[entity];

			if (componentIndex == Constants::Entities::InvalidComponentID)
			{
				EB_CORE_WARN("Entity {} does not contain component type!", entity);
				return;
			}

			// Swap-and-pop: overwrite the removed slot with the last element to stay packed
			uint32_t lastComponentIndex = static_cast<uint32_t>(DenseComponentArray.size() - 1);
			uint32_t entityReplaceId = DenseEntityArray[lastComponentIndex];

			DenseComponentArray[componentIndex] = DenseComponentArray[lastComponentIndex];

			// Fix up the sparse mapping for the moved element
			DenseEntityArray[componentIndex] = entityReplaceId;
			SparseEntityArray[entityReplaceId] = componentIndex;

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

		inline void DetachComponent(EntityID entity, ComponentType type)
		{
			if (type < m_ComponentArrays.size() && m_ComponentArrays[type] != nullptr)
			{
				m_ComponentArrays[type]->RemoveComponent(entity);
			}
		}

		template<typename T>
		inline ComponentType GetComponentType()
		{
			// Each unique T gets a monotonically increasing ID, cached after first call
			static const ComponentType typeId = GetNextComponentType();
			return typeId;
		}

		template<typename T>
		inline T& GetComponent(EntityID entity)
		{
			ComponentType type = GetComponentType<T>();

			EB_CORE_ASSERT(type < m_ComponentArrays.size() && m_ComponentArrays[type] != nullptr, "Component type has never been registered in this scene!");
			SharedPtr<ComponentMemoryArray<T>> memoryArrays = StaticPointerCast<ComponentMemoryArray<T>>(m_ComponentArrays[type]);
			return memoryArrays->GetComponent(entity);
		}

		inline void* GetComponentDataErased(EntityID entity, ComponentType type)
		{
			if (type >= m_ComponentArrays.size() || m_ComponentArrays[type] == nullptr)
			{
				EB_CORE_WARN("Component type {} does not exist!", type);
				return nullptr;
			}
			return m_ComponentArrays[type]->GetComponentDataErased(entity);
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