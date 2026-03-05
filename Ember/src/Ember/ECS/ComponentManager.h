#pragma once

#include "Types.h"
#include "Entity.h"

#include "Ember/Core/Core.h"

#include <vector>
#include <concepts>

namespace Ember {

	struct ComponentMemoryArraysBase 
	{
		virtual ~ComponentMemoryArraysBase() = default;
		virtual void EntityDestroyed(Entity entity) = 0;
	};

	template<typename T>
	struct ComponentMemoryArray : public ComponentMemoryArrayBase
	{
		std::vector<unsigned int> SparseEntityArray;
		std::vector<unsigned int> EntityComponentMapper;
		std::vector<T> ComponentArrayDense;

		ComponentMemoryArray()
		{
			SparseEntityArray.resize(EB_MAX_ENTITIES, EB_INVALID_ENTITY_ID);
		}

		virtual void EntityDestroyed(Entity entity) override
		{
			// TODO: Implement swap and pop logic
		}
	};

	class ComponentManager
	{
	public:
		template<typename T>
		void AddComponent(Entity entity, T component);

		void RemoveComponent(Entity entity, ComponentType type);
		void EntityDestroyed(Entity entity);

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