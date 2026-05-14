#pragma once

#include "RaycastData.h"
#include "Ember/Core/Filter.h"

namespace Ember {

	class Raycast
	{
	public:
		static RaycastData CastRay(const Vector3f& startPoint, const Vector3f& endPoint, Filter filter = FilterPreset::All);
	};

}