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
		std::array<float, GamepadAxisArraySize> Axis = {};
		bool Connected = false;
	};

}