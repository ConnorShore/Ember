#pragma once

#include "GamepadCode.h"

#include <cstdint>
#include <array>

namespace Ember {

	static constexpr size_t GamepadAxisArraySize = static_cast<size_t>(GamepadAxis::Last);
	using GamepadButtonMask = uint16_t;

	struct GamepadState
	{
		GamepadButtonMask Down = 0;
		GamepadButtonMask PreviousDown = 0;

		// Deadzoned, curved and inverted by Input::ProcessGamepads - what everything reads.
		std::array<float, GamepadAxisArraySize> Axis = {};

		// What the OS reported, kept so conditioning never shapes its own output a second time.
		std::array<float, GamepadAxisArraySize> RawAxis = {};

		bool Connected = false;
	};

}