#include "Ember.h"

class SandboxApp : public Ember::Application
{
public:
	SandboxApp()
	{
	}
	~SandboxApp()
	{
	}
};

Ember::Application* Ember::CreateApplication()
{
	return new SandboxApp();
}