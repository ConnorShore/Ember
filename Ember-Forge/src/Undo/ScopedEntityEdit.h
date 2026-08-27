#pragma once

#include <Ember/Asset/UUID.h>
#include <Ember/Scene/EntitySnapshot.h>

#include <string>
#include <vector>

namespace Ember {

	struct EditorContext;

	// Brackets a structural editor action - create, delete, duplicate, reparent, reorder - so it
	// lands on the undo stack as a single entry. Captures on construction, diffs on destruction.
	class ScopedEntityEdit
	{
	public:
		ScopedEntityEdit(EditorContext& context, std::string label, const std::vector<UUID>& affected,
			bool includeDescendants = true);
		~ScopedEntityEdit();

		ScopedEntityEdit(const ScopedEntityEdit&) = delete;
		ScopedEntityEdit& operator=(const ScopedEntityEdit&) = delete;

		// Records an entity the action created. It did not exist at capture time, so undo must
		// delete it and redo must bring it back.
		void AddCreated(UUID entityUUID);

		void Cancel() { m_Cancelled = true; }

	private:
		EditorContext& m_Context;
		std::string m_Label;
		std::vector<UUID> m_Affected;
		std::vector<UUID> m_SelectionBefore;
		EntitySetSnapshot m_Before;
		bool m_IncludeDescendants;
		bool m_Cancelled = false;
		bool m_Active = false;
	};

}
