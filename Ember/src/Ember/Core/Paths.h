#pragma once

#include <filesystem>

namespace Ember::Paths {

	// Resolves the locations the engine reads from and writes to, so that a build installed into a
	// fixed (possibly read-only) directory works without every call site hardcoding the repo layout.
	//
	// Two layouts are supported. An installed build is flat and anchored to the executable:
	//
	//     Ember-Forge.exe / Ember-Runtime.exe / EmberCore/ / EditorAssets/
	//
	// A development build keeps the repo layout and resolves against the working directory, which
	// Premake's `debugdir` pins to the workspace root. Dev-mode results are deliberately identical
	// to the literals these paths replaced, so the existing workflow is unaffected.

	// Directory containing the running executable.
	const std::filesystem::path& ExecutableDir();

	// True when the executable sits beside a staged EmberCore, which only an installed build does.
	bool IsInstalled();

	// Engine-owned assets: shaders, default textures, Lua script templates.
	const std::filesystem::path& EngineAssets();

	// Editor-owned assets: panel icons, fonts, branding.
	const std::filesystem::path& EditorAssets();

	// The standalone player, used for playtesting and as the payload for an exported project.
	const std::filesystem::path& RuntimeExe();

	// %LOCALAPPDATA%. Exposed because a shipped game's save directory is keyed on the project name
	// rather than on Ember, so it cannot go under UserDataDir().
	const std::filesystem::path& LocalAppData();

	// %APPDATA%, for state that should roam with the user profile.
	const std::filesystem::path& RoamingAppData();

	// Per-user writable state that should survive an uninstall but not roam (layout, logs, caches).
	const std::filesystem::path& UserDataDir();

	// Per-user configuration that should roam with the user profile (recent projects).
	const std::filesystem::path& UserConfigDir();

	// Logs every resolved location. Call it once at startup, after file logging is initialised - when
	// assets fail to load, these four lines say immediately whether path resolution is the reason.
	void LogResolved();

}
