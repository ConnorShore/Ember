#include "efpch.h"
#include "ScopedEntityEdit.h"

#include "EditorContext.h"
#include "UndoStack.h"

#include <Ember/Scene/Scene.h>

#include <algorithm>

namespace Ember {

	ScopedEntityEdit::ScopedEntityEdit(EditorContext& context, std::string label, const std::vector<UUID>& affected,
		bool includeDescendants)
		: m_Context(context), m_Label(std::move(label)), m_Affected(affected), m_IncludeDescendants(includeDescendants)
	{
		if (m_Context.CurrentSceneState != SceneState::Edit)
			return;

		auto scene = m_Context.ActiveScene();
		if (!scene || !m_Context.ActiveUndoStack())
			return;

		m_Active = true;
		m_SelectionBefore = m_Context.SelectionUUIDs();
		m_Before = EntitySetSnapshot::Capture(scene, m_Affected, m_IncludeDescendants);
	}

	void ScopedEntityEdit::AddCreated(UUID entityUUID)
	{
		if (!m_Active || entityUUID == Constants::InvalidUUID)
			return;

		if (std::find(m_Affected.begin(), m_Affected.end(), entityUUID) == m_Affected.end())
			m_Affected.push_back(entityUUID);

		// It genuinely did not exist when we captured, so the "before" state is its absence.
		m_Before.MarkAbsent(entityUUID);
	}

	ScopedEntityEdit::~ScopedEntityEdit()
	{
		if (!m_Active || m_Cancelled)
			return;

		auto scene = m_Context.ActiveScene();
		UndoStack* undoStack = m_Context.ActiveUndoStack();
		if (!scene || !undoStack)
			return;

		EntitySetSnapshot after = EntitySetSnapshot::Capture(scene, m_Affected, m_IncludeDescendants);
		if (after.Equals(m_Before))
			return;

		EditAction action;
		action.Label = std::move(m_Label);
		action.Before = std::move(m_Before);
		action.After = std::move(after);
		action.SelectionBefore = std::move(m_SelectionBefore);
		action.SelectionAfter = m_Context.SelectionUUIDs();

		undoStack->Push(std::move(action));
	}

}
