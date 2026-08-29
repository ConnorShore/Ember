#pragma once

#include "Ember/Input/Input.h"
#include "Ember/Input/InputCode.h"

namespace Ember {
	namespace Windows {

		class Input
		{
		public:
			static KeyCode GlfwKeyCodeToEmberKeyCode(int key);

			// GLFW reports up to eight physical buttons, so this yields a MouseControl rather than a
			// MouseButton - the latter only names the first three and would drop the side buttons.
			// Returns false for a code outside the range, since there is no "unknown" control to
			// report and defaulting to Left would fake a click.
			static bool GlfwMouseButtonToEmberMouseControl(int button, MouseControl& out);
		};

	}
}
