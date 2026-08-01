#include <Ember.h>
#include <Ember/Core/EntryPoint.h>

#include "TestFramework.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>

namespace Ember {

	namespace {

		// Command-line flags take precedence over the equivalent environment variables.
		//   Ember-Test.exe --filter=unit --run=Physics --repeat=3 --xml=Profiles/tests.xml
		std::string ReadOption(const char* flagPrefix, const char* envName)
		{
			auto& app = Application::Instance();
			for (int i = 1; i < app.GetCommandLineArgsCount(); ++i)
			{
				const char* arg = app.GetCommandLineArg(i);
				if (!arg)
					continue;

				const std::string text(arg);
				const std::string prefix(flagPrefix);
				if (text.rfind(prefix, 0) == 0)
					return text.substr(prefix.size());
			}

			if (const char* fromEnv = std::getenv(envName))
				return std::string(fromEnv);

			return std::string();
		}

		bool HasFlag(const char* flag, const char* envName)
		{
			auto& app = Application::Instance();
			for (int i = 1; i < app.GetCommandLineArgsCount(); ++i)
			{
				const char* arg = app.GetCommandLineArg(i);
				if (arg && std::string(arg) == flag)
					return true;
			}
			return std::getenv(envName) != nullptr;
		}

	} // namespace

	// Runs the suite on the first frame, once the window, GL context, systems and default assets are up.
	// Exits 0/1 for CI; std::exit skips engine shutdown so a teardown crash can't mask the result.
	class TestRunnerLayer : public Layer
	{
	public:
		TestRunnerLayer() : Layer("TestRunnerLayer") {}

		void OnUpdate(TimeStep delta) override
		{
			if (m_Ran)
				return;
			m_Ran = true;

			if (HasFlag("--list", "EMBER_TEST_LIST"))
			{
				Ember::Test::ListTests();
				std::exit(0);
			}

			PrepareProject();
			PrepareRenderTargets();

			Ember::Test::RunOptions options;

			const std::string typeFilter = ReadOption("--filter=", "EMBER_TEST_FILTER");
			if (!typeFilter.empty())
			{
				m_TypeFilterStorage = typeFilter;
				options.TypeFilter = m_TypeFilterStorage.c_str();
			}

			options.NameFilter = ReadOption("--run=", "EMBER_TEST_RUN");

			const std::string repeat = ReadOption("--repeat=", "EMBER_TEST_REPEAT");
			if (!repeat.empty())
			{
				const int parsed = std::atoi(repeat.c_str());
				options.Repeat = parsed > 0 ? parsed : 1;
			}

			options.XmlOutputPath = ReadOption("--xml=", "EMBER_TEST_XML");
			options.PerfCsvPath = ReadOption("--perf-csv=", "EMBER_TEST_PERF_CSV");

			// Always on: names each test just before it runs, flushed, so a hard crash (an engine
			// assert firing __debugbreak, an access violation inside a vendored library) still leaves
			// behind an unambiguous record of which test was executing.
			options.BreadcrumbPath = "Logs/test-progress.log";

			const int failed = Ember::Test::RunAll(options);

			std::fflush(stdout);
			std::exit(failed == 0 ? 0 : 1);
		}

	private:
		// Parts of the engine assume an active Project without checking - ScriptEngine::BindAPI reaches
		// through ProjectManager::GetActive() at bind time - so the runner stands up a throwaway one.
		// Safe to do before the suite runs: ClearAssets() only drops non-engine assets.
		void PrepareProject()
		{
			if (ProjectManager::GetActive())
				return;

			const std::filesystem::path projectDirectory = "Ember-Test/tmp/EmberTestProject";

			// NewProject serializes the .ebproj before it creates any directories, so the parent has
			// to exist first or the write silently does nothing.
			std::error_code ec;
			std::filesystem::create_directories(projectDirectory, ec);

			const std::filesystem::path projectFile = projectDirectory / "EmberTestProject.ebproj";
			ProjectManager::NewProject(projectFile.string());

			if (!ProjectManager::GetActive())
			{
				EB_CORE_ERROR("Ember-Test: failed to create the throwaway project at '{}'. "
					"Scripting tests that call ScriptEngine::BindAPI will crash.", projectFile.string());
			}
		}

		// Several tests render a real frame - the visual tests directly, and any test that calls
		// SceneFixture::TickEdit (the only way to flush Scene's deferred entity removals). Sizing the
		// deferred render targets to the window once, up front, means none of them has to.
		void PrepareRenderTargets()
		{
			auto& app = Application::Instance();
			const uint32_t width = app.GetWindow().GetWidth();
			const uint32_t height = app.GetWindow().GetHeight();

			if (auto renderSystem = app.GetSystem<RenderSystem>())
			{
				renderSystem->OnViewportResize(width, height);
				RenderAction::SetViewport(0, 0, width, height);
			}
		}

	private:
		bool m_Ran = false;
		std::string m_TypeFilterStorage; // RunOptions::TypeFilter is a borrowed pointer
	};

	class EmberTestApp : public Application
	{
	public:
		EmberTestApp(const ApplicationSpecification& spec)
			: Application(spec)
		{
			PushLayer(ScopedPtr<Layer>(new TestRunnerLayer()));
		}
	};

	ScopedPtr<Application> CreateApplication(int argc, char** argv)
	{
		Logger::InitFileLogging("Logs/test.txt");
		EB_CORE_INFO("Ember-Test starting...");

		ApplicationSpecification spec;
		spec.Name = "Ember-Test";
		spec.WindowSpecification.Width = 1280;
		spec.WindowSpecification.Height = 720;
		spec.WindowSpecification.Title = "Ember-Test";

		// debugdir is the workspace root, so the engine's source assets are under Ember/assets. If
		// the default shaders/textures fail to load, this path is the first thing to check - the
		// Assets::DefaultEngineAssetsAreLoaded test reports exactly that failure.
		spec.EngineAssetDir = "Ember/assets";
		spec.ProjectAssetDir = "Ember/assets";

		spec.CommandLineArgsCount = argc;
		spec.CommandLineArgs = argv;

		return ScopedPtr<EmberTestApp>(new EmberTestApp(spec));
	}
}
