#pragma once

#define EB_MAX_ENTITIES 100
#define EB_MAX_COMPONENTS 64

#define EB_INVALID_ENTITY_ID EB_MAX_ENTITIES + 1

using EntityID = unsigned int;
using ComponentType = unsigned int;