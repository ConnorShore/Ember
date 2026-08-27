#pragma once

#include <filesystem>

namespace Ember {

	enum class GizmoPivotMode
	{
		ActiveEntity = 0,
		SelectionCenter
	};

	// Per-user authoring preferences, stored outside any project so they follow the user rather than
	// the level being edited.
	struct EditorPreferences
	{
		// Snapping defaults to on because assembling modular kit pieces is the common case; the
		// toolbar toggle and a held Ctrl both invert it.
		bool SnapEnabled = true;
		float TranslateSnap = 1.0f;
		float RotateSnap = 90.0f;
		float ScaleSnap = 0.1f;

		bool GizmoLocalSpace = false;
		GizmoPivotMode PivotMode = GizmoPivotMode::ActiveEntity;
		bool SpawnAtCursor = true;

		static std::filesystem::path FilePath();

		bool Save() const;
		bool Load();
	};

}
