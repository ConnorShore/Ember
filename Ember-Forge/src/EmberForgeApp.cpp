#include "efpch.h"

#include <Ember.h>
#include <Ember/Core/EntryPoint.h>
#include <Ember/Core/Paths.h>

#include "EditorLayer.h"

namespace Ember {

	class EmberForgeApp : public Application
	{
	public:
		EmberForgeApp(const ApplicationSpecification& spec)
			: Application(spec)
		{
			PushLayer(ScopedPtr<Layer>(new EditorLayer()));
		}
	};

	ScopedPtr<Application> CreateApplication(int argc, char** argv)
	{
		// Dist builds are windowed and so have no console; without this the shipped editor would have
		// nowhere to report a failure.
		Logger::InitFileLogging((Paths::UserDataDir() / "Logs" / "editor.txt").string());
		Paths::LogResolved();

		ApplicationSpecification spec;
		spec.Name = "Ember Forge";
		spec.WindowSpecification.Title = "Ember Forge";
		spec.WindowSpecification.Width = 1600;
		spec.WindowSpecification.Height = 900;
		spec.WindowSpecification.StartMaximized = true;

		// Resolved rather than hardcoded so the editor also runs from a flat install directory.
		spec.EngineAssetDir = Paths::EngineAssets();
		spec.ProjectAssetDir = Paths::EditorAssets();

		// Forwarded so EditorLayer can open a .ebproj handed to us by the shell association.
		spec.CommandLineArgsCount = argc;
		spec.CommandLineArgs = argv;

		return ScopedPtr<EmberForgeApp>(new EmberForgeApp(spec));
	}

}
