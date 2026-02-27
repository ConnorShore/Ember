#pragma once

#include "Core.h"
#include "Application.h"

#ifdef EB_PLATFORM_WINDOWS

extern Ember::Application* Ember::CreateApplication();

int main(int argc, char** argv)
{
	auto app = Ember::CreateApplication();
	app->Run();
	delete app;
	return 0;
}

#else
#error Only Windows is supported!
#endif