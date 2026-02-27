#pragma once

#ifdef EB_ENABLE_ASSERTS

#define EB_ASSERT(x, ...) { if(!(x)) { EB_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#define EB_CORE_ASSERT(x, ...) { if(!(x)) { EB_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }	

#else
#define EB_ASSERT(...)
#define EB_CORE_ASSERT(...)
#endif