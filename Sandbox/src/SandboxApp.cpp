#include <Ember.h>
#include <Ember/Core/EntryPoint.h>

#include "SpriteTestLayer.h"
#include "PBRTestLayer.h"

class SandboxApp : public Ember::Application
{
public:
	SandboxApp()
	{
		PushLayer(Ember::ScopedPtr<Ember::Layer>(new PBRTestLayer()));
		//PushLayer(Ember::ScopedPtr<Ember::Layer>(new SpriteTestLayer()));
	}
	~SandboxApp()
	{
	}
};

Ember::ScopedPtr<Ember::Application> Ember::CreateApplication()
{
	return Ember::ScopedPtr<SandboxApp>(new SandboxApp());
}