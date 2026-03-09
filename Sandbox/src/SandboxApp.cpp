#include <Ember.h>
#include <Ember/Core/EntryPoint.h>

#include "SandboxLayer.h"
#include "EntityTestLayer.h"
#include "SceneTestLayer.h"

class SandboxApp : public Ember::Application
{
public:
	SandboxApp()
	{
		//PushLayer(Ember::ScopedPtr<Ember::Layer>(new SandboxLayer()));
		//PushLayer(Ember::ScopedPtr<Ember::Layer>(new EntityTestLayer()));
		PushLayer(Ember::ScopedPtr<Ember::Layer>(new SceneTestLayer()));
	}
	~SandboxApp()
	{
	}
};

Ember::Application* Ember::CreateApplication()
{
	return new SandboxApp();
}