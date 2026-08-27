#pragma once

#include "Ember/Asset/UUID.h"
#include "Ember/Core/Core.h"

#include <string>
#include <vector>

namespace Ember {

	class Entity;
	class Scene;

	// One entity's serialized state plus where it sat in the hierarchy, so a restore can put it back
	// exactly rather than appending it somewhere new.
	struct EntitySnapshot
	{
		UUID EntityUUID = Constants::InvalidUUID;
		UUID ParentUUID = Constants::InvalidUUID;

		// Index among the parent's children, or among the scene's root entities when unparented.
		int SiblingIndex = -1;

		// False means the entity did not exist when this was captured, so restoring it means deleting.
		bool Existed = false;

		std::string YAML;
	};

	// A capture of a set of entities that can be restored onto a scene. This is the undo primitive:
	// it reuses the prefab serializer, which already round-trips every component and repairs
	// parent/child links through a UUID remap.
	class EntitySetSnapshot
	{
	public:
		// Captures `roots`, optionally pulling in each one's descendants. Entities that do not exist
		// are recorded as absent so a restore removes anything created since.
		static EntitySetSnapshot Capture(const SharedPtr<Scene>& scene, const std::vector<UUID>& roots, bool includeDescendants);

		void Restore(const SharedPtr<Scene>& scene) const;

		// String comparison over the captured YAML, so an edit that ends where it started is dropped
		// rather than pushed as an empty undo step.
		bool Equals(const EntitySetSnapshot& other) const;

		bool Empty() const { return m_Snapshots.empty(); }
		size_t ByteSize() const;

		// Records that an entity did not exist at capture time, so undo deletes it again.
		void MarkAbsent(UUID entityUUID);

	private:
		std::vector<EntitySnapshot> m_Snapshots;
	};

}
