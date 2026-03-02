#include <Ember.h>
#include <Ember/Core/EntryPoint.h>

#include "SandboxLayer.h"
#include "GuiLayer.h"

class SandboxApp : public Ember::Application
{
public:
	SandboxApp()
	{
		PushLayer(Ember::ScopedPtr<Ember::Layer>(new SandboxLayer()));
		PushCanvasLayer(Ember::ScopedPtr<Ember::Layer>(new GuiLayer()));
	}
	~SandboxApp()
	{
	}
};

Ember::Application* Ember::CreateApplication()
{
	return new SandboxApp();
}