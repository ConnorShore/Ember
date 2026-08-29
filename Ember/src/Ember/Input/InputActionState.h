#pragma once

#include "InputDevice.h"

namespace Ember {

	struct InputActionState
	{
		bool IsDown = false;
		bool JustPressed = false;
		bool JustReleased = false;

		InputDevice LastDevice = InputDevice::None;
	};

}