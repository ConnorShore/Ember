#pragma once

#include "Ember/ECS/Types.h"
#include "Ember/Core/Filter.h"
#include "Ember/Core/Constants.h"

#include <cstdint>

namespace Ember {

	struct ColliderUserData
	{
		EntityID EntityID = Constants::Entities::InvalidEntityID;
		Filter Filter = FilterPreset::Default;
	};

	// EntityID <-> rp3d user data (void*)
	// ----------------------------------
	// ReactPhysics3D stores an opaque void* per body and per collider, and Ember uses it to carry the
	// owning EntityID. The obvious encoding - cast the ID straight to a pointer - is BROKEN for
	// entity 0: it produces a null pointer, which is indistinguishable from "this collider has no
	// user data at all" (true of the camera sensor and of the temporary probe bodies that overlap
	// queries create). Every reader null-checks before decoding, so entity 0 was silently dropped
	// from overlap results and reported as InvalidEntityID by raycasts.
	//
	// The ID is therefore stored BIASED BY ONE, so entity 0 encodes as pointer value 1 and null keeps
	// its "no owner" meaning. Always go through these two helpers - never cast directly.
	inline void* EncodeEntityUserData(EntityID entity)
	{
		return reinterpret_cast<void*>(static_cast<uintptr_t>(entity) + 1u);
	}

	// Returns InvalidEntityID when the user data is null (no owning entity).
	inline EntityID DecodeEntityUserData(void* userData)
	{
		if (userData == nullptr)
			return static_cast<EntityID>(Constants::Entities::InvalidEntityID);

		return static_cast<EntityID>(reinterpret_cast<uintptr_t>(userData) - 1u);
	}

	inline bool HasEntityUserData(void* userData)
	{
		return userData != nullptr;
	}

}