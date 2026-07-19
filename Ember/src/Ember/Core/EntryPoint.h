#pragma once

#include "Core.h"
#include "Application.h"
#include "Ember/Performance/Profiler.h"

#include <exception>

#ifdef EB_PLATFORM_WINDOWS

extern Ember::ScopedPtr<Ember::Application> Ember::CreateApplication(int argc, char** argv);

int main(int argc, char** argv)
{
	try
	{
		// Profiles/ is a sibling of Logs/ at the workspace root (Premake debugdir).
		EB_PROFILE_BEGIN_SESSION("Startup", "Profiles/Startup.json");
		auto app = Ember::CreateApplication(argc, argv);
		app->OnAttach();
		EB_PROFILE_END_SESSION();

		EB_PROFILE_BEGIN_SESSION("Runtime", "Profiles/Runtime.json");
		app->Run();
		EB_PROFILE_END_SESSION();

		EB_PROFILE_BEGIN_SESSION("Shutdown", "Profiles/Shutdown.json");
		app->OnDetach();
		EB_PROFILE_END_SESSION();

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