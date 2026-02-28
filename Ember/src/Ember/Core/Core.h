#pragma once

#ifdef _WIN32
	#ifdef _WIN64
		#define EB_PLATFORM_WINDOWS
	#else
		#error "x86 Builds are not supported!"
	#endif
#else
	#error "Unknown platform!"
#endif

#if defined(EB_DEBUG)
#define EB_ENABLE_ASSERTS
#endif

#include "Assert.h"
#include "Logger.h"
#include "SharedPointer.h"
#include "ScopedPointer.h"