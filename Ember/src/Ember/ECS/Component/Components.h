#pragma once

#include "Ember/Math/Math.h"

#define EB_DEFAULT_COMPONENT_CONSTRUCT(type) type() = default; \
                                                type(const type& other) = default; \
                                                ~type() = default

namespace Ember {

	struct TransformComponent
	{
		Vector3f Transform;

		EB_DEFAULT_COMPONENT_CONSTRUCT(TransformComponent);
	};

	struct RigidBodyComponent
	{
		Vector3f velocity;

		EB_DEFAULT_COMPONENT_CONSTRUCT(RigidBodyComponent);
	};

}