#pragma once

#include "EditAction.h"

#include <Ember/Core/Core.h>

#include <string>
#include <vector>

namespace Ember {

	class Scene;

	// Bounded undo history for one scene. Entries past the cursor are the redo tail and are dropped
	// as soon as a new action is pushed.
	class UndoStack
	{
	public:
		UndoStack(size_t maxEntries = 200, size_t maxBytes = 32ull * 1024ull * 1024ull)
			: m_MaxEntries(maxEntries), m_MaxBytes(maxBytes) {}

		void Push(EditAction&& action);

		bool CanUndo() const { return m_Cursor > 0; }
		bool CanRedo() const { return m_Cursor < m_Actions.size(); }

		const std::string& PeekUndoLabel() const;
		const std::string& PeekRedoLabel() const;

		bool Undo(const SharedPtr<Scene>& scene, std::vector<UUID>& outSelection);
		bool Redo(const SharedPtr<Scene>& scene, std::vector<UUID>& outSelection);

		void Clear();

	private:
		void TrimToBudget();

	private:
		std::vector<EditAction> m_Actions;

		// Everything below the cursor is undoable; everything from it up is redoable.
		size_t m_Cursor = 0;

		size_t m_MaxEntries;
		size_t m_MaxBytes;
	};

}
