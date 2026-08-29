#include "ebpch.h"
#include "Input.h"

#include "Ember/Core/Core.h"

#include <GLFW/glfw3.h>

namespace Ember {
	namespace Windows {

		// Maps GLFW key codes to Ember key codes. They currently match, so no conversion is needed.
		KeyCode Input::GlfwKeyCodeToEmberKeyCode(int key)
		{
			// Last is the array size rather than a key, so it is out of range, not the upper bound.
			if (key < 0 || key >= static_cast<int>(KeyCode::Last))
			{
				EB_CORE_ASSERT(false, "Undefined key code: {}", key);
				return KeyCode::Unknown;
			}

			return static_cast<KeyCode>(key);
		}

		// Maps GLFW mouse button codes to Ember codes. They currently match, so no conversion is needed.
		bool Input::GlfwMouseButtonToEmberMouseControl(int button, MouseControl& out)
		{
			// GLFW_MOUSE_BUTTON_1..8 land on Left, Right, Middle and Button4..Button8.
			if (button < 0 || button >= static_cast<int>(MouseControl::Last))
			{
				EB_CORE_ASSERT(false, "Undefined mouse button code: {0}", button);
				return false;
			}

			out = static_cast<MouseControl>(button);
			return true;
		}

	}
}
