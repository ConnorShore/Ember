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

private:

};

Ember::Application* Ember::CreateApplication()
{
	return new SandboxApp();
}