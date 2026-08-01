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

	// EntityID is stored in rp3d's per-body/per-collider void* BIASED BY ONE, so entity 0 encodes as
	// pointer value 1 and null keeps its "no owning entity" meaning. Casting the ID directly made
	// entity 0 indistinguishable from "no user data" and silently dropped it from every query.
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