#pragma once

#include "Core.h"
#include "Application.h"

#include <exception>

#ifdef EB_PLATFORM_WINDOWS

extern Ember::ScopedPtr<Ember::Application> Ember::CreateApplication(int argc, char** argv);

int main(int argc, char** argv)
{
	try
	{
		auto app = Ember::CreateApplication(argc, argv);
		app->OnAttach();
		app->Run();
		app->OnDetach();
		return 0;
	}
	catch (const std::exception& e)
	{
		EB_CORE_FATAL("Unhandled exception while starting application: {}", e.what());
		return -1;
	}
}

#else
#error Only Windows is supported!
#endif