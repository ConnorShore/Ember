#pragma once

#include "Ember/Input/Input.h"
#include "Ember/Input/InputCode.h"

namespace Ember {
	namespace Windows {

		class Input
		{
		public:
			static KeyCode GlfwKeyCodeToEmberKeyCode(int key);
			static MouseButton GlfwMouseButtonToEmberMouseButton(int button);
		};

	}
}