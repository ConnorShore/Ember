#include <Ember.h>
#include <Ember/Core/EntryPoint.h>

#include "EditorLayer.h"

class EmberForgeApp : public Ember::Application
{
public:
	EmberForgeApp()
	{
		PushLayer(Ember::ScopedPtr<Ember::Layer>(new EditorLayer()));
	}
	~EmberForgeApp()
	{
	}
};

Ember::ScopedPtr<Ember::Application> Ember::CreateApplication()
{
	return Ember::ScopedPtr<EmberForgeApp>(new EmberForgeApp());
}