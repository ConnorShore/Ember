#include "ebpch.h"

#include "Paths.h"

namespace Ember::Paths {

	namespace {

		// The build config folder Premake emits, used to locate sibling binaries in a dev tree.
		constexpr const char* ConfigFolder()
		{
#if defined(EB_DEBUG)
			return "Debug-windows-x86_64";
// Profile defines EB_RELEASE as well, so it has to be tested first or it would resolve to the
// Release output directory.
#elif defined(EB_PROFILE)
			return "Profile-windows-x86_64";
#elif defined(EB_RELEASE)
			return "Release-windows-x86_64";
#elif defined(EB_DIST)
			return "Dist-windows-x86_64";
#else
			return "Debug-windows-x86_64";
#endif
		}

		std::filesystem::path ResolveExecutableDir()
		{
#ifdef EB_PLATFORM_WINDOWS
			// Wide API because the install path can contain non-ASCII characters (a user profile name).
			std::wstring buffer(MAX_PATH, L'\0');
			while (true)
			{
				const DWORD written = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
				if (written == 0)
				{
					EB_CORE_ERROR("GetModuleFileNameW failed, falling back to the working directory");
					return std::filesystem::current_path();
				}

				// A full buffer means the path was truncated, so grow it and ask again.
				if (written == static_cast<DWORD>(buffer.size()))
				{
					buffer.resize(buffer.size() * 2);
					continue;
				}

				buffer.resize(written);
				return std::filesystem::path(buffer).parent_path();
			}
#else
			return std::filesystem::current_path();
#endif
		}

		// Resolves a Windows known folder from the environment rather than the shell APIs, matching how
		// the engine already looked these up before Paths existed.
		std::filesystem::path ResolveKnownFolder(const char* variable)
		{
			std::filesystem::path resolved;

#ifdef EB_PLATFORM_WINDOWS
			char* value = nullptr;
			size_t size = 0;
			if (_dupenv_s(&value, &size, variable) == 0 && value != nullptr)
			{
				resolved = value;
				free(value);
			}
#endif

			// A missing variable would otherwise leave an empty path and send writes to the CWD, which
			// for an installed build is the install directory.
			if (resolved.empty())
			{
				resolved = std::filesystem::temp_directory_path();
				EB_CORE_ERROR("%{}% is not set, falling back to {}", variable, resolved.string());
			}

			return resolved;
		}

		// User directories are created up front because every caller writes into them.
		std::filesystem::path EnsureDirectory(std::filesystem::path directory)
		{
			std::error_code error;
			std::filesystem::create_directories(directory, error);
			if (error)
				EB_CORE_ERROR("Could not create user directory {}: {}", directory.string(), error.message());

			return directory;
		}

		// Everything is resolved together, once, in plain statements. Deriving each accessor from its
		// own lazily-initialised static meant three of them depended on two others being initialised
		// first, and they were the three that came back empty; one value with no interdependencies
		// removes that whole class of problem.
		struct Layout
		{
			std::filesystem::path ExecutableDir;
			bool Installed = false;
			std::filesystem::path EngineAssets;
			std::filesystem::path EditorAssets;
			std::filesystem::path RuntimeExe;
			std::filesystem::path LocalAppData;
			std::filesystem::path RoamingAppData;
			std::filesystem::path UserData;
			std::filesystem::path UserConfig;
		};

		Layout ResolveLayout()
		{
			Layout layout;

			layout.ExecutableDir = ResolveExecutableDir();

			// error_code overload so a transient filesystem failure cannot throw out of initialisation.
			std::error_code error;
			layout.Installed = std::filesystem::exists(layout.ExecutableDir / "EmberCore", error) && !error;

			if (layout.Installed)
			{
				layout.EngineAssets = layout.ExecutableDir / "EmberCore";
				layout.EditorAssets = layout.ExecutableDir / "EditorAssets";
				layout.RuntimeExe = layout.ExecutableDir / "Ember-Runtime.exe";
			}
			else
			{
				// Resolved against the working directory, which Premake's debugdir pins to the workspace
				// root. These are exactly the literals that used to be hardcoded at each call site.
				layout.EngineAssets = std::filesystem::path("Ember/assets");
				layout.EditorAssets = std::filesystem::path("Ember-Forge/assets");
				layout.RuntimeExe = std::filesystem::path("bin") / ConfigFolder() / "Ember-Runtime" / "Ember-Runtime.exe";
			}

			layout.LocalAppData = ResolveKnownFolder("LOCALAPPDATA");
			layout.RoamingAppData = ResolveKnownFolder("APPDATA");
			layout.UserData = EnsureDirectory(layout.LocalAppData / "EmberForge");
			layout.UserConfig = EnsureDirectory(layout.RoamingAppData / "Ember-Forge");

			return layout;
		}

		const Layout& Get()
		{
			static const Layout s_Layout = ResolveLayout();
			return s_Layout;
		}

	}

	const std::filesystem::path& ExecutableDir() { return Get().ExecutableDir; }
	bool IsInstalled()                           { return Get().Installed; }
	const std::filesystem::path& EngineAssets()  { return Get().EngineAssets; }
	const std::filesystem::path& EditorAssets()  { return Get().EditorAssets; }
	const std::filesystem::path& RuntimeExe()    { return Get().RuntimeExe; }
	const std::filesystem::path& LocalAppData()  { return Get().LocalAppData; }
	const std::filesystem::path& RoamingAppData(){ return Get().RoamingAppData; }
	const std::filesystem::path& UserDataDir()   { return Get().UserData; }
	const std::filesystem::path& UserConfigDir() { return Get().UserConfig; }

	void LogResolved()
	{
		const Layout& layout = Get();

		EB_CORE_INFO("Paths: installed={} exe={}", layout.Installed, layout.ExecutableDir.string());
		EB_CORE_INFO("Paths: engineAssets={} editorAssets={}", layout.EngineAssets.string(), layout.EditorAssets.string());
		EB_CORE_INFO("Paths: runtimeExe={}", layout.RuntimeExe.string());
		EB_CORE_INFO("Paths: userData={} userConfig={}", layout.UserData.string(), layout.UserConfig.string());
	}

}
