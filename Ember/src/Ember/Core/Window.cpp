#include "ebpch.h"
#include "Window.h"
#include "Core.h"

#ifdef EB_PLATFORM_WINDOWS
#include "Ember/Platform/Windows/Window.h"
#else
#error Only windows is currently supported
#endif

namespace Ember {


	ScopedPtr<Window> Window::Create(const WindowConfig& config)
	{
#ifdef EB_PLATFORM_WINDOWS
		return ScopedPtr<Windows::Window>::Create(config);
#else
		EB_CORE_ASSERT(false, "Only windows is supported currently");
		return nullptr;
#endif
	}
}