#include <Ember.h>
#include <Ember/Core/EntryPoint.h>

#include "SandboxLayer.h"
#include "EntityTestLayer.h"

class SandboxApp : public Ember::Application
{
public:
	SandboxApp()
	{
		PushLayer(Ember::ScopedPtr<Ember::Layer>(new SandboxLayer()));
		//PushLayer(Ember::ScopedPtr<Ember::Layer>(new EntityTestLayer()));
	}
	~SandboxApp()
	{
	}
};

Ember::Application* Ember::CreateApplication()
{
	return new SandboxApp();
}