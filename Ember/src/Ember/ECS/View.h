#pragma once

#include "Ember/ECS/Types.h"

#include <vector>

namespace Ember {

	// TODO: Move to dynamic sorting based on the type with the smallest component array
	// However for now, the Driver will automatically be the type we loop over for simplicity
	// Need to come back and revisit this after the whole ECS is working

	class Registry;

	template<typename Driver, typename... Filters>
	class View
	{
	public:
		View(Registry* registry)
			: m_Registry(registry) {
		}
		~View() = default;

		struct Iterator
		{
		public:
			Registry* m_Registry;
			const std::vector<EntityID>* m_Entities;
			unsigned int m_Index;

			Iterator(Registry* registry, const std::vector<EntityID>* entities, unsigned int index)
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
				unsigned int entityCt = m_Entities->size();
				while (m_Index < entityCt)
				{
					if (m_Registry->ContainsComponents<Filters...>((*m_Entities)[m_Index]))
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
			return Iterator(m_Registry, &driverEntities, driverEntities.size());
		}

	private:
		Registry* m_Registry;
	};
}