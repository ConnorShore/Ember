#include "efpch.h"
#include "EditorPreferences.h"

#include <Ember/Core/Paths.h>
#include <Ember/Math/Math.h>
#include <Ember/Utils/SerializationUtils.h>

#include <ryml.hpp>
#include <ryml_std.hpp>

#include <fstream>
#include <sstream>

namespace Ember {

	namespace {

		// Keeps a malformed or hand-edited file from putting the gizmo into an unusable state.
		float SanitizeSnap(float value, float fallback)
		{
			return (value > 0.0f && Math::IsFinite(value)) ? value : fallback;
		}

	}

	std::filesystem::path EditorPreferences::FilePath()
	{
		return Paths::UserConfigDir() / "EditorPreferences.yaml";
	}

	bool EditorPreferences::Save() const
	{
		ryml::Tree tree;
		ryml::NodeRef root = tree.rootref();
		root |= ryml::MAP;

		auto snapNode = root["Snap"];
		snapNode |= ryml::MAP;
		snapNode["Enabled"] << (SnapEnabled ? 1 : 0);
		snapNode["Translate"] << TranslateSnap;
		snapNode["Rotate"] << RotateSnap;
		snapNode["Scale"] << ScaleSnap;

		auto gizmoNode = root["Gizmo"];
		gizmoNode |= ryml::MAP;
		gizmoNode["LocalSpace"] << (GizmoLocalSpace ? 1 : 0);
		gizmoNode["PivotMode"] << static_cast<int>(PivotMode);

		auto placementNode = root["Placement"];
		placementNode |= ryml::MAP;
		placementNode["SpawnAtCursor"] << (SpawnAtCursor ? 1 : 0);

		std::ofstream fout(FilePath());
		if (!fout.is_open())
		{
			EB_CORE_ERROR("Failed to open editor preferences for writing: {}", FilePath().string());
			return false;
		}

		fout << tree;
		fout.close();

		return true;
	}

	bool EditorPreferences::Load()
	{
		std::ifstream stream(FilePath());
		if (!stream.is_open())
			return false;

		std::stringstream strStream;
		strStream << stream.rdbuf();
		std::string yamlData = strStream.str();

		ryml::Tree tree = ryml::parse_in_arena(ryml::to_csubstr(yamlData));
		ryml::NodeRef root = tree.rootref();
		if (!root.is_map())
			return false;

		// Every field is optional so an older preferences file keeps its defaults rather than failing.
		if (root.has_child("Snap"))
		{
			auto snapNode = root["Snap"];
			int enabled = SnapEnabled ? 1 : 0;
			Util::ReadField(snapNode, "Enabled", enabled);
			SnapEnabled = enabled != 0;
			Util::ReadField(snapNode, "Translate", TranslateSnap);
			Util::ReadField(snapNode, "Rotate", RotateSnap);
			Util::ReadField(snapNode, "Scale", ScaleSnap);
		}

		if (root.has_child("Gizmo"))
		{
			auto gizmoNode = root["Gizmo"];
			int localSpace = GizmoLocalSpace ? 1 : 0;
			int pivotMode = static_cast<int>(PivotMode);
			Util::ReadField(gizmoNode, "LocalSpace", localSpace);
			GizmoLocalSpace = localSpace != 0;
			Util::ReadField(gizmoNode, "PivotMode", pivotMode);
			PivotMode = pivotMode == static_cast<int>(GizmoPivotMode::SelectionCenter)
				? GizmoPivotMode::SelectionCenter
				: GizmoPivotMode::ActiveEntity;
		}

		if (root.has_child("Placement"))
		{
			auto placementNode = root["Placement"];
			int spawnAtCursor = SpawnAtCursor ? 1 : 0;
			Util::ReadField(placementNode, "SpawnAtCursor", spawnAtCursor);
			SpawnAtCursor = spawnAtCursor != 0;
		}

		TranslateSnap = SanitizeSnap(TranslateSnap, 1.0f);
		RotateSnap = SanitizeSnap(RotateSnap, 90.0f);
		ScaleSnap = SanitizeSnap(ScaleSnap, 0.1f);

		return true;
	}

}
