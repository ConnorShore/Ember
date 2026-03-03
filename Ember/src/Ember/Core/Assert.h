#pragma once

#ifdef EB_ENABLE_ASSERTS

	#define EB_ASSERT(x, ...) { if(!(x)) { EB_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }

	#ifdef EB_ENGINE
		#define EB_CORE_ASSERT(x, ...) { if(!(x)) { EB_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
	#endif

#else
	#define EB_ASSERT(...)

	#ifdef EB_ENGINE
		#define EB_CORE_ASSERT(...)
	#endif
#endif