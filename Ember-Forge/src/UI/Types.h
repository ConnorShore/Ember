#pragma once

#include <functional>

namespace Ember {
	namespace UI {

		using UICallbackFunc = std::function<void()>;
		using UICallbackFuncBool = std::function<void(bool)>;

	}
}