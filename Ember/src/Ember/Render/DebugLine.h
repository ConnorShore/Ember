#pragma once

#include "Ember/math/Math.h"

namespace Ember {

	struct DebugVertex {
		Vector3f Position;
		Vector4f Color;
	};

	// Must match the layout of rp3d::DebugRenderer::DebugLine for easy conversion
	struct DebugLine
	{
		Vector3f Point1;
		uint32_t Color1;
		Vector3f Point2;
		uint32_t Color2;
	};

}