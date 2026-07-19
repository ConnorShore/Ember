#pragma once

#include "Instrumentor.h"

#ifdef EB_PROFILE
	#define EB_PROFILE_ENABLED 1
#else
	#define EB_PROFILE_ENABLED 0
#endif

#define EB_PROFILE_CONCAT_INTERNAL(x, y) x##y
#define EB_PROFILE_CONCAT(x, y) EB_PROFILE_CONCAT_INTERNAL(x, y)

#if EB_PROFILE_ENABLED
	#define EB_PROFILE_BEGIN_SESSION(name, filepath) ::Ember::Instrumentor::Get().BeginSession(name, filepath)
	#define EB_PROFILE_END_SESSION() ::Ember::Instrumentor::Get().EndSession()
	#define EB_PROFILE_SCOPE(name) ::Ember::InstrumentationTimer EB_PROFILE_CONCAT(timer, __LINE__)(name)
	#define EB_PROFILE_FUNCTION() EB_PROFILE_SCOPE(__FUNCSIG__)
#else
	#define EB_PROFILE_BEGIN_SESSION(name, filepath)
	#define EB_PROFILE_END_SESSION()
	#define EB_PROFILE_SCOPE(name)
	#define EB_PROFILE_FUNCTION()
#endif
