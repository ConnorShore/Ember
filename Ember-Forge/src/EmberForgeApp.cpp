#include <Ember.h>
#include <Ember/Core/EntryPoint.h>

#include "EditorLayer.h"

namespace Ember {

	class EmberForgeApp : public Application
	{
	public:
		EmberForgeApp()
			: Application("Ember Forge", WindowConfig("Ember Forge", 1600, 900))
		{
			PushLayer(ScopedPtr<Layer>(new EditorLayer()));
		}
		~EmberForgeApp()
		{
		}
	};

	ScopedPtr<Application> CreateApplication()
	{
		return ScopedPtr<EmberForgeApp>(new EmberForgeApp());
	}

}
