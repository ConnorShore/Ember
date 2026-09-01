#pragma once

#include "InputTrigger.h"

#include <string>
#include <vector>

namespace Ember {

	struct InputAction
	{
		std::string Name;
		std::vector<InputTrigger> Triggers;
	};

}