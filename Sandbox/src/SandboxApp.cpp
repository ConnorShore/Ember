#include <Ember.h>
#include <Ember/Core/EntryPoint.h>

#include "SceneTestLayer.h"

class SandboxApp : public Ember::Application
{
public:
	SandboxApp()
	{
		PushLayer(Ember::ScopedPtr<Ember::Layer>(new SceneTestLayer()));
	}
	~SandboxApp()
	{
	}
};

Ember::ScopedPtr<Ember::Application> Ember::CreateApplication()
{
	return Ember::ScopedPtr<SandboxApp>(new SandboxApp());
}