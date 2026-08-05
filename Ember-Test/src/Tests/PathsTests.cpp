// Ember::Paths resolution, and the invariants that let an installed (possibly read-only) build work.
// These run in a dev tree, so IsInstalled() is false here and the dev branch is what gets exercised.

#include <Ember.h>

#include "TestFramework.h"

#include <filesystem>
#include <fstream>
#include <string>

using namespace Ember;
using Ember::Test::Type::Unit;

//////////////////////////////////////////////////////////////////////////
// Executable and layout detection
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Paths, ExecutableDirIsAbsoluteAndExists, Unit)
{
	const auto& dir = Paths::ExecutableDir();

	EB_CHECK(!dir.empty());
	EB_CHECK_MSG(dir.is_absolute(), dir.string());
	EB_CHECK_MSG(std::filesystem::exists(dir), dir.string());

	// Ember-Test.exe lives in bin/<Config>-windows-x86_64/Ember-Test.
	EB_EXPECT_EQ(dir.filename().string(), std::string("Ember-Test"));
}

EB_TEST_CASE(Paths, TestTreeIsNotDetectedAsInstalled, Unit)
{
	// The marker is a staged EmberCore beside the executable, which only Stage.bat produces. If this
	// ever trips in a dev tree, every asset path silently switches to the installed layout.
	EB_CHECK_FALSE(Paths::IsInstalled());
	EB_CHECK_FALSE(std::filesystem::exists(Paths::ExecutableDir() / "EmberCore"));
}

//////////////////////////////////////////////////////////////////////////
// Asset directories
//////////////////////////////////////////////////////////////////////////

// Locks in the "dev mode is byte-identical to the old hardcoded literals" guarantee that keeps the
// existing debugdir-at-workspace-root workflow working.
EB_TEST_CASE(Paths, DevAssetDirsMatchTheRepoLayout, Unit)
{
	EB_CHECK_EQ(Paths::EngineAssets(), std::filesystem::path("Ember/assets"));
	EB_CHECK_EQ(Paths::EditorAssets(), std::filesystem::path("Ember-Forge/assets"));
}

EB_TEST_CASE(Paths, EngineAssetsContainsTheDefaultShaders, Unit)
{
	const auto shader = Paths::EngineAssets() / "shaders" / "StandardLit.glsl";
	EB_CHECK_MSG(std::filesystem::exists(shader), std::filesystem::absolute(shader).string());
}

EB_TEST_CASE(Paths, RuntimeExeIsNamedForTheRuntime, Unit)
{
	// Dev builds point into bin/<Config>/...; installed builds sit flat beside the editor. Either way
	// the filename is what the editor launches and what project export copies.
	EB_CHECK_EQ(Paths::RuntimeExe().filename().string(), std::string("Ember-Runtime.exe"));
}

//////////////////////////////////////////////////////////////////////////
// User directories
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Paths, UserDirsResolveUnderTheKnownFolders, Unit)
{
	EB_CHECK(!Paths::LocalAppData().empty());
	EB_CHECK(!Paths::RoamingAppData().empty());

	EB_CHECK_EQ(Paths::UserDataDir(), Paths::LocalAppData() / "EmberForge");
	EB_CHECK_EQ(Paths::UserConfigDir(), Paths::RoamingAppData() / "Ember-Forge");

	// The accessors create these, because every caller writes into them.
	EB_CHECK(std::filesystem::exists(Paths::UserDataDir()));
	EB_CHECK(std::filesystem::exists(Paths::UserConfigDir()));
}

EB_TEST_CASE(Paths, UserDataDirIsWritable, Unit)
{
	const auto probe = Paths::UserDataDir() / "paths-write-probe.tmp";

	{
		std::ofstream stream(probe);
		EB_CHECK_MSG(stream.is_open(), probe.string());
		stream << "probe";
	}

	EB_CHECK(std::filesystem::exists(probe));

	std::error_code error;
	std::filesystem::remove(probe, error);
	EB_EXPECT_FALSE(static_cast<bool>(error));
}

// Regression guard for the shutdown write that used to land in Ember/assets/assets.eba. The
// destructor only runs after the suite finishes, so its effect cannot be observed from a test -
// what is checkable is the invariant that made it a bug: writable state must not live inside the
// install tree, which for an installed build is read-only for a standard user.
EB_TEST_CASE(Paths, WritableStateLivesOutsideTheInstallTree, Unit)
{
	const auto exeDir = std::filesystem::absolute(Paths::ExecutableDir()).lexically_normal();
	const auto engineAssets = std::filesystem::absolute(Paths::EngineAssets()).lexically_normal();

	// A relative path from the install root that does not start with ".." means the writable directory
	// sits inside it. An empty result means no relative path exists at all, which is also outside.
	const auto isOutsideOf = [](const std::filesystem::path& candidate, const std::filesystem::path& root)
	{
		const auto relative = candidate.lexically_relative(root);
		return relative.empty() || relative.begin()->string() == "..";
	};

	for (const auto& writable : { Paths::UserDataDir(), Paths::UserConfigDir() })
	{
		const auto normalized = std::filesystem::absolute(writable).lexically_normal();

		EB_EXPECT_MSG(isOutsideOf(normalized, exeDir), normalized.string() + " is inside " + exeDir.string());
		EB_EXPECT_MSG(isOutsideOf(normalized, engineAssets), normalized.string() + " is inside " + engineAssets.string());
	}
}
