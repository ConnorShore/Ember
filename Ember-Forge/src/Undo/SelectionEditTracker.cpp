#include "efpch.h"
#include "SelectionEditTracker.h"

#include "EditorContext.h"
#include "UndoStack.h"

#include <Ember/Performance/Profiler.h>
#include <Ember/Scene/Scene.h>

#include <imgui/imgui.h>

namespace Ember {

	void SelectionEditTracker::OnFrameEnd(EditorContext& context, bool gizmoActive)
	{
		EB_PROFILE_FUNCTION();

		if (context.CurrentSceneState != SceneState::Edit)
		{
			Reset();
			return;
		}

		auto scene = context.ActiveScene();
		UndoStack* undoStack = context.ActiveUndoStack();
		if (!scene || !undoStack)
		{
			Reset();
			return;
		}

		// IsAnyItemActive stays true for a whole drag and for as long as a text field holds focus,
		// which is exactly the window an edit should be coalesced over.
		bool interacting = ImGui::IsAnyItemActive() || gizmoActive;

		if (interacting)
		{
			m_WasInteracting = true;
			return;
		}

		if (m_WasInteracting)
		{
			m_WasInteracting = false;

			if (!m_BaselineSelection.empty())
			{
				EntitySetSnapshot after = EntitySetSnapshot::Capture(scene, m_BaselineSelection, false);
				if (!after.Equals(m_Baseline))
				{
					EditAction action;
					action.Label = "Edit";
					action.Before = m_Baseline;
					action.After = std::move(after);
					action.SelectionBefore = m_BaselineSelection;
					action.SelectionAfter = context.SelectionUUIDs();

					undoStack->Push(std::move(action));
				}
			}
		}

		// Re-baseline while idle so the next interaction has something to diff against.
		m_BaselineSelection = context.SelectionUUIDs();
		m_Baseline = m_BaselineSelection.empty()
			? EntitySetSnapshot()
			: EntitySetSnapshot::Capture(scene, m_BaselineSelection, false);
	}

	void SelectionEditTracker::Reset()
	{
		m_WasInteracting = false;
		m_BaselineSelection.clear();
		m_Baseline = EntitySetSnapshot();
	}

}
