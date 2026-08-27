#include "ebpch.h"
#include "EntitySnapshot.h"

#include "Ember/ECS/Component/Components.h"
#include "Ember/Scene/Entity.h"
#include "Ember/Scene/Scene.h"
#include "Ember/Scene/SceneSerializer.h"

#include <algorithm>

namespace Ember {

	namespace {

		// Where the entity sits among its siblings, so a restore can put it back at the same index
		// rather than appending it to the end of the list.
		int FindSiblingIndex(const SharedPtr<Scene>& scene, Entity entity, UUID parentUUID)
		{
			if (parentUUID != Constants::InvalidUUID)
			{
				Entity parent = scene->GetEntity(parentUUID);
				if (parent.GetEntityHandle() == Constants::Entities::InvalidEntityID || !parent.ContainsComponent<RelationshipComponent>())
					return -1;

				const auto& children = parent.GetComponent<RelationshipComponent>().Children;
				auto it = std::find(children.begin(), children.end(), entity.GetUUID());
				return it == children.end() ? -1 : static_cast<int>(it - children.begin());
			}

			const auto& order = scene->GetEntityOrder();
			auto it = std::find(order.begin(), order.end(), entity.GetUUID());
			return it == order.end() ? -1 : static_cast<int>(it - order.begin());
		}

	}

	EntitySetSnapshot EntitySetSnapshot::Capture(const SharedPtr<Scene>& scene, const std::vector<UUID>& roots, bool includeDescendants)
	{
		EntitySetSnapshot snapshot;
		if (!scene)
			return snapshot;

		SceneSerializer serializer(scene);

		for (UUID rootUUID : roots)
		{
			EntitySnapshot entry;
			entry.EntityUUID = rootUUID;

			Entity entity = scene->GetEntity(rootUUID);
			if (entity.GetEntityHandle() == Constants::Entities::InvalidEntityID)
			{
				// Absent at capture time, so restoring this state means removing it again.
				snapshot.m_Snapshots.push_back(std::move(entry));
				continue;
			}

			entry.Existed = true;

			if (entity.ContainsComponent<RelationshipComponent>())
				entry.ParentUUID = entity.GetComponent<RelationshipComponent>().ParentHandle;

			entry.SiblingIndex = FindSiblingIndex(scene, entity, entry.ParentUUID);

			std::vector<Entity> captured = includeDescendants
				? serializer.GatherSubtree(entity)
				: std::vector<Entity>{ entity };

			entry.YAML = serializer.SerializeEntitiesToString(captured, entity.GetName());
			snapshot.m_Snapshots.push_back(std::move(entry));
		}

		return snapshot;
	}

	void EntitySetSnapshot::Restore(const SharedPtr<Scene>& scene) const
	{
		if (!scene)
			return;

		SceneSerializer serializer(scene);

		// Entities that did not exist when this was captured have to go first, so a later recreate
		// cannot collide with one of them.
		bool removedAny = false;
		for (const EntitySnapshot& entry : m_Snapshots)
		{
			if (entry.Existed)
				continue;

			Entity existing = scene->GetEntity(entry.EntityUUID);
			if (existing.GetEntityHandle() != Constants::Entities::InvalidEntityID)
			{
				scene->RemoveEntity(existing);
				removedAny = true;
			}
		}

		// Restoring happens inside one call, so it cannot wait for the end-of-frame drain.
		if (removedAny)
			scene->FlushPendingRemovals();

		for (const EntitySnapshot& entry : m_Snapshots)
		{
			if (!entry.Existed)
				continue;

			// Clear anything the entity has picked up since, so components removed by the action
			// being undone do not survive the restore.
			Entity existing = scene->GetEntity(entry.EntityUUID);
			if (existing.GetEntityHandle() != Constants::Entities::InvalidEntityID)
				scene->ResetEntityToCoreComponents(existing);

			std::vector<Entity> restored = serializer.DeserializeEntitiesFromString(entry.YAML, true);
			if (restored.empty())
				continue;

			Entity root = restored.front();

			// The captured YAML holds the parent link but not the parent's own child list, so a
			// parent outside the captured set has to be re-pointed at this entity by hand.
			if (entry.ParentUUID != Constants::InvalidUUID)
			{
				Entity parent = scene->GetEntity(entry.ParentUUID);
				if (parent.GetEntityHandle() != Constants::Entities::InvalidEntityID && parent.ContainsComponent<RelationshipComponent>())
				{
					auto& children = parent.GetComponent<RelationshipComponent>().Children;
					auto it = std::find(children.begin(), children.end(), entry.EntityUUID);
					if (it == children.end())
					{
						size_t index = entry.SiblingIndex >= 0
							? std::min(static_cast<size_t>(entry.SiblingIndex), children.size())
							: children.size();
						children.insert(children.begin() + index, entry.EntityUUID);
					}
				}
			}
			else if (entry.SiblingIndex >= 0)
			{
				scene->SetRootEntityIndex(entry.EntityUUID, static_cast<size_t>(entry.SiblingIndex));
			}

			for (Entity entity : restored)
			{
				if (entity.ContainsComponent<TransformComponent>())
					entity.GetComponent<TransformComponent>().InvalidateWorld();
			}
		}
	}

	bool EntitySetSnapshot::Equals(const EntitySetSnapshot& other) const
	{
		if (m_Snapshots.size() != other.m_Snapshots.size())
			return false;

		for (size_t i = 0; i < m_Snapshots.size(); i++)
		{
			const EntitySnapshot& a = m_Snapshots[i];
			const EntitySnapshot& b = other.m_Snapshots[i];

			if (a.EntityUUID != b.EntityUUID || a.Existed != b.Existed || a.ParentUUID != b.ParentUUID
				|| a.SiblingIndex != b.SiblingIndex || a.YAML != b.YAML)
				return false;
		}

		return true;
	}

	size_t EntitySetSnapshot::ByteSize() const
	{
		size_t bytes = 0;
		for (const EntitySnapshot& entry : m_Snapshots)
			bytes += entry.YAML.size() + sizeof(EntitySnapshot);

		return bytes;
	}

	void EntitySetSnapshot::MarkAbsent(UUID entityUUID)
	{
		for (EntitySnapshot& entry : m_Snapshots)
		{
			if (entry.EntityUUID != entityUUID)
				continue;

			entry.Existed = false;
			entry.YAML.clear();
			return;
		}

		EntitySnapshot entry;
		entry.EntityUUID = entityUUID;
		m_Snapshots.push_back(std::move(entry));
	}

}
