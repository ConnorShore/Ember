#pragma once

#include <Ember/Asset/UUID.h>
#include <Ember/Scene/EntitySnapshot.h>

#include <string>
#include <vector>

namespace Ember {

	// One undoable editor action, stored as the affected entities' state on either side of it plus
	// the selection, so undoing also puts the user back where they were.
	struct EditAction
	{
		std::string Label;

		EntitySetSnapshot Before;
		EntitySetSnapshot After;

		std::vector<UUID> SelectionBefore;
		std::vector<UUID> SelectionAfter;
	};

}
