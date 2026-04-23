#pragma once
#include "Ember/ECS/Types.h"
#include <vector>

namespace Ember {

	// TODO: Move to dynamic sorting based on the type with the smallest component array
	// However for now, the Driver will automatically be the type we loop over for simplicity
	// Need to come back and revisit this after the whole ECS is working

	class Registry;

	// Base Template (Notice how it uses 3 strict types now, no loose packs!)
	template<typename Driver, typename FilterTuple, typename ExcludeType>
	class View;

	// Partial Specialization (Unpacks the std::tuple and Exclude struct)
	template<typename Driver, typename... Filters, typename... Excludes>
	class View<Driver, std::tuple<Filters...>, Exclude<Excludes...>>
	{
	public:
		View(Registry* registry)
			: m_Registry(registry) {
		}
		~View() = default;

		EntityID Front() const
		{
			for (EntityID entity : m_Registry->GetActiveEntities<Driver>())
			{
				bool hasFilters = true;
				if constexpr (sizeof...(Filters) > 0)
					hasFilters = m_Registry->ContainsComponents<Filters...>(entity);
				bool hasExcludes = false;
				if constexpr (sizeof...(Excludes) > 0)
					hasExcludes = (m_Registry->ContainsComponent<Excludes>(entity) || ...);
				if (hasFilters && !hasExcludes)
					return entity;
			}
			return Constants::Entities::InvalidEntityID;
		}

		bool Empty() const
		{
			return Front() == Constants::Entities::InvalidEntityID;
		}

		struct Iterator
		{
		public:
			Registry* m_Registry;
			const std::vector<EntityID>* m_Entities;
			uint32_t m_Index;

			Iterator(Registry* registry, const std::vector<EntityID>* entities, uint32_t index)
				: m_Registry(registry), m_Entities(entities), m_Index(index)
			{
				FindValidEntities();
			}

			EntityID operator*() const { return (*m_Entities)[m_Index]; }
			Iterator& operator++()
			{
				m_Index++;
				FindValidEntities();
				return *this;
			}

			bool operator==(const Iterator& other) const { return m_Index == other.m_Index && m_Entities == other.m_Entities; }
			bool operator!=(const Iterator& other) const { return m_Index != other.m_Index || m_Entities != other.m_Entities; }

		private:
			void FindValidEntities()
			{
				uint32_t entityCt = static_cast<uint32_t>(m_Entities->size());
				while (m_Index < entityCt)
				{
					EntityID currentEntity = (*m_Entities)[m_Index];

					// 1. Check if the entity has all the required Filters
					bool hasFilters = true;
					if constexpr (sizeof...(Filters) > 0)
						hasFilters = m_Registry->ContainsComponents<Filters...>(currentEntity);

					// 2. Check if the entity has ANY of the Excluded components using a C++17 Fold Expression
					bool hasExcludes = false;
					if constexpr (sizeof...(Excludes) > 0)
						hasExcludes = (m_Registry->ContainsComponent<Excludes>(currentEntity) || ...);

					// If it has what we want, and DOES NOT have what we exclude, we found a match!
					if (hasFilters && !hasExcludes)
						break;

					m_Index++;
				}
			}
		};

		Iterator begin() const
		{
			return Iterator(m_Registry, &m_Registry->GetActiveEntities<Driver>(), 0);
		}

		Iterator end() const
		{
			const auto& driverEntities = m_Registry->GetActiveEntities<Driver>();
			return Iterator(m_Registry, &driverEntities, static_cast<uint32_t>(driverEntities.size()));
		}

	private:
		Registry* m_Registry;
	};
}