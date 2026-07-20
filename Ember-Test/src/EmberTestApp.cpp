#include <Ember.h>
#include <Ember/Core/EntryPoint.h>

#include "TestFramework.h"

#include <cstdlib>

namespace Ember {

	// Runs the whole suite on the first frame — by then the window, GL context, every system, and the
	// default engine assets are fully initialized, so unit, integration, AND visual tests can all run
	// from one place. Exits with 0 (all passed) or 1 (any failure) for CI.
	//
	// Optional env vars:
	//   EMBER_TEST_FILTER=unit|integration|visual|performance   -> run only that category
	//   EMBER_TEST_WRITE_GOLDEN=1                                -> (re)generate visual golden images
	class TestRunnerLayer : public Layer
	{
	public:
		TestRunnerLayer() : Layer("TestRunnerLayer") {}

		void OnUpdate(TimeStep delta) override
		{
			if (m_Ran)
				return;
			m_Ran = true;

			const char* filter = std::getenv("EMBER_TEST_FILTER");
			const int failed = Ember::Test::RunAll(filter);

			std::exit(failed == 0 ? 0 : 1);
		}

	private:
		bool m_Ran = false;
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
		EB_CORE_INFO("Ember-Test starting...");
		Logger::InitFileLogging("Logs/test.txt");

		ApplicationSpecification spec;
		spec.Name = "Ember-Test";
		spec.WindowSpecification.Width = 1280;
		spec.WindowSpecification.Height = 720;
		spec.WindowSpecification.Title = "Ember-Test";

		// debugdir is the workspace root, so the engine's source assets are under Ember/assets. If the
		// default shaders/textures fail to load, this path is the first thing to check.
		spec.EngineAssetDir = "Ember/assets";
		spec.ProjectAssetDir = "Ember/assets";

		spec.CommandLineArgsCount = argc;
		spec.CommandLineArgs = argv;

		return ScopedPtr<EmberTestApp>(new EmberTestApp(spec));
	}
}
