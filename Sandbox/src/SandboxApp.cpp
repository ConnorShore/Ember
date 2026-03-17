#include <Ember.h>
#include <Ember/Core/EntryPoint.h>

#include "SpriteTestLayer.h"
#include "DeferredShadingLayer.h"
#include "ModelTestLayer.h"

class SandboxApp : public Ember::Application
{
public:
	SandboxApp()
	{
		PushLayer(Ember::ScopedPtr<Ember::Layer>(new ModelTestLayer()));
		//PushLayer(Ember::ScopedPtr<Ember::Layer>(new DeferredShadingLayer()));
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