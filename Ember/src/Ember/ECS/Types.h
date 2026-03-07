#pragma once

#define EB_MAX_ENTITIES 128
#define EB_MAX_COMPONENTS 64

#define EB_INVALID_ENTITY_ID EB_MAX_ENTITIES + 1
#define EB_INVALID_COMPONENT_ID EB_MAX_COMPONENTS + 1

namespace Ember {
	using EntityID = unsigned int;
	using ComponentType = unsigned int;
}
