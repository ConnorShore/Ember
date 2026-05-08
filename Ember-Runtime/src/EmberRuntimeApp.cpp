#include <Ember.h>
#include <Ember/Core/EntryPoint.h>

#include "RuntimeLayer.h"

namespace Ember {
	class EmberRuntimeApp : public Application
	{
	public:
		EmberRuntimeApp()
			: Application("My Ember Game", WindowConfig("My Ember Game", 1600, 900))
		{
			PushLayer(ScopedPtr<Layer>(new RuntimeLayer()));
		}
		~EmberRuntimeApp()
		{
		}
	};
	ScopedPtr<Application> CreateApplication()
	{
		return ScopedPtr<EmberRuntimeApp>(new EmberRuntimeApp());
	}
}