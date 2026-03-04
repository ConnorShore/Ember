#include <Ember.h>
#include <Ember/Core/EntryPoint.h>

#include "SandboxLayer.h"

class SandboxApp : public Ember::Application
{
public:
	SandboxApp()
	{
		PushLayer(Ember::ScopedPtr<Ember::Layer>(new SandboxLayer()));
	}
	~SandboxApp()
	{
	}
};

Ember::Application* Ember::CreateApplication()
{
	return new SandboxApp();
}