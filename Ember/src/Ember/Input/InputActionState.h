#pragma once

#include "InputDevice.h"
#include "InputTrigger.h"

namespace Ember {

	struct InputActionState
	{
		bool IsDown = false;
		bool JustPressed = false;
		bool JustReleased = false;
		float Strength = 0.0f;

		InputDevice LastDevice = InputDevice::None;

		// The control behind LastDevice, so a consumer can silence the one that actually fired rather
		// than every trigger the action is bound to.
		InputControlId LastControl = KeyCode::Unknown;

		// Set when the UI acts on a press before gameplay ever sees it. It has to outlive the frame
		// it was set in: the control is still held on the next Evaluate, which would otherwise read
		// the suppressed IsDown as a fresh edge and hand gameplay the press one frame late.
		bool Consumed = false;

		// The unsuppressed reading, kept so edge detection stays anchored to the real control while
		// IsDown reports what gameplay is allowed to see.
		bool RawIsDown = false;
	};

}