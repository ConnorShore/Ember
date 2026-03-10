#pragma once

#include "Core.h"
#include "Application.h"

#ifdef EB_PLATFORM_WINDOWS

extern Ember::ScopedPtr<Ember::Application> Ember::CreateApplication();

int main(int argc, char** argv)
{
	auto app = Ember::CreateApplication();
	app->OnAttach();
	app->Run();
	app->OnDetach();
	return 0;
}

#else
#error Only Windows is supported!
#endif