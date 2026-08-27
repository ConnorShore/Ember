#pragma once

#include <Ember/Asset/UUID.h>
#include <Ember/Scene/EntitySnapshot.h>

#include <vector>

namespace Ember {

	struct EditorContext;

	// Snapshots the selection whenever the editor is idle, so whatever interaction follows collapses
	// into a single undo entry. This covers gizmo drags, sliders, checkboxes, colour pickers and
	// rename fields uniformly, without every widget having to report its own before and after.
	class SelectionEditTracker
	{
	public:
		// Call once at the end of the frame, after the UI has been submitted.
		void OnFrameEnd(EditorContext& context, bool gizmoActive);

		void Reset();

	private:
		bool m_WasInteracting = false;
		std::vector<UUID> m_BaselineSelection;
		EntitySetSnapshot m_Baseline;
	};

}
