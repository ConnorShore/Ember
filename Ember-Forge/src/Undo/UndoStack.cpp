#include "efpch.h"
#include "UndoStack.h"

#include <Ember/Scene/Scene.h>

namespace Ember {

	namespace {
		const std::string s_NoLabel;
	}

	void UndoStack::Push(EditAction&& action)
	{
		// A new action invalidates whatever was redoable.
		m_Actions.resize(m_Cursor);

		m_Actions.push_back(std::move(action));
		m_Cursor = m_Actions.size();

		TrimToBudget();
	}

	const std::string& UndoStack::PeekUndoLabel() const
	{
		return CanUndo() ? m_Actions[m_Cursor - 1].Label : s_NoLabel;
	}

	const std::string& UndoStack::PeekRedoLabel() const
	{
		return CanRedo() ? m_Actions[m_Cursor].Label : s_NoLabel;
	}

	bool UndoStack::Undo(const SharedPtr<Scene>& scene, std::vector<UUID>& outSelection)
	{
		if (!CanUndo() || !scene)
			return false;

		const EditAction& action = m_Actions[--m_Cursor];
		action.Before.Restore(scene);
		outSelection = action.SelectionBefore;

		return true;
	}

	bool UndoStack::Redo(const SharedPtr<Scene>& scene, std::vector<UUID>& outSelection)
	{
		if (!CanRedo() || !scene)
			return false;

		const EditAction& action = m_Actions[m_Cursor++];
		action.After.Restore(scene);
		outSelection = action.SelectionAfter;

		return true;
	}

	void UndoStack::Clear()
	{
		m_Actions.clear();
		m_Cursor = 0;
	}

	void UndoStack::TrimToBudget()
	{
		size_t bytes = 0;
		for (const EditAction& action : m_Actions)
			bytes += action.Before.ByteSize() + action.After.ByteSize();

		// Oldest entries go first, which is the only direction that keeps the redo tail intact.
		while (m_Actions.size() > m_MaxEntries || (bytes > m_MaxBytes && m_Actions.size() > 1))
		{
			bytes -= m_Actions.front().Before.ByteSize() + m_Actions.front().After.ByteSize();
			m_Actions.erase(m_Actions.begin());

			if (m_Cursor > 0)
				m_Cursor--;
		}
	}

}
