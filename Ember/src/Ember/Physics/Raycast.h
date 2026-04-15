#pragma once

#include "RaycastData.h"

namespace Ember {

	class Raycast
	{
	public:
		static RaycastData CastRay(const Vector3f& startPoint, const Vector3f& endPoint);
	};

}