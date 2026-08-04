#include "efpch.h"

#include <Ember.h>
#include <Ember/Core/EntryPoint.h>

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
		ApplicationSpecification spec;
		spec.Name = "Ember Forge";
		spec.WindowSpecification.Title = "Ember Forge";
		//spec.WindowSpecification.IconPath = "Ember-Forge/assets/images/EmberIcon.png";
		spec.WindowSpecification.Width = 1600;
		spec.WindowSpecification.Height = 900;
		spec.WindowSpecification.StartMaximized = true;

		// The Editor explicitly points to the source code folders
		spec.EngineAssetDir = "Ember/assets";
		spec.ProjectAssetDir = "Ember-Forge/assets";

		return ScopedPtr<EmberForgeApp>(new EmberForgeApp(spec));
	}

}
