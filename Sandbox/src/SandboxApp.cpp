#include <Ember.h>
#include <Ember/Core/EntryPoint.h>

#include "SceneTestLayer.h"
#include "Test3DLayer.h"

class SandboxApp : public Ember::Application
{
public:
	SandboxApp()
	{
		//PushLayer(Ember::ScopedPtr<Ember::Layer>(new SceneTestLayer()));
		PushLayer(Ember::ScopedPtr<Ember::Layer>(new Test3DLayer()));
	}
	~SandboxApp()
	{
	}
};

Ember::ScopedPtr<Ember::Application> Ember::CreateApplication()
{
	return Ember::ScopedPtr<SandboxApp>(new SandboxApp());
}