#pragma once

#include "InputDevice.h"

namespace Ember {

	struct InputActionState
	{
		bool IsDown = false;
		bool JustPressed = false;
		bool JustReleased = false;
		float Strength = 0.0f;

		InputDevice LastDevice = InputDevice::None;
	};

}