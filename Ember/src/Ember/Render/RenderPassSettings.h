#pragma once

#include "Camera.h"
#include "Ember/Core/Filter.h"
#include "Ember/Math/Math.h"

namespace Ember {

	struct RenderPassSettings
	{
		Camera* ActiveCamera;
		Matrix4f CameraTransform;
		Filter RenderMask;
		Filter VolumeMask;
	};

}