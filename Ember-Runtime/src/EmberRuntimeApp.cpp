#include <Ember.h>
#include <Ember/Core/EntryPoint.h>

#include "RuntimeLayer.h"

namespace Ember {
	class EmberRuntimeApp : public Application
	{
	public:
		EmberRuntimeApp(const ApplicationSpecification& spec)
			: Application(spec)
		{
			PushLayer(ScopedPtr<Layer>(new RuntimeLayer()));
		}
		~EmberRuntimeApp()
		{
		}
	};
	ScopedPtr<Application> CreateApplication(int argc, char** argv)
	{
		EB_CORE_INFO("Standalone Runtime Starting...");
		Logger::InitFileLogging("Logs/runtime.txt");

		ApplicationSpecification spec;
		spec.Name = "My Ember Game";
		spec.WindowSpecification.Width = 1600;
		spec.WindowSpecification.Height = 900;
		spec.WindowSpecification.Title = "My Ember Game";

		std::string engineAssetDir = "EmberCore";
		if (argc >= 2) {
			engineAssetDir = argv[2];
		}

		std::string projectAssetDir = "GameData";
		if (argc >= 3) {
			projectAssetDir = argv[3];
		}

		// The Editor explicitly points to the source code folders
		spec.EngineAssetDir = engineAssetDir;
		spec.ProjectAssetDir = projectAssetDir;

		spec.CommandLineArgsCount = argc;
		spec.CommandLineArgs = argv;

		return ScopedPtr<EmberRuntimeApp>(new EmberRuntimeApp(spec));
	}
}