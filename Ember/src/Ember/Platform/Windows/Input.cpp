#include "ebpch.h"
#include "Input.h"

#include "Ember/Core/Core.h"

#include <GLFW/glfw3.h>

namespace Ember {
	namespace Windows {

		// Maps GLFW key codes to Ember key codes. They currently match, so no conversion is needed.
		KeyCode Input::GlfwKeyCodeToEmberKeyCode(int key)
		{
			if (key < 0 || KeyCode::Last < key)
			{
				EB_CORE_ASSERT(false, "Undefined key code: {}", key);
				return KeyCode::Unknown;
			}

			return static_cast<KeyCode>(key);
		}

		// Maps GLFW mouse button codes to Ember codes. They currently match, so no conversion is needed.
		Ember::MouseButton Input::GlfwMouseButtonToEmberMouseButton(int button)
		{
			if (MouseButton::Last < button)
			{
				EB_CORE_ASSERT(false, "Undefined mouse button code: {0}", button);
				return MouseButton::Unknown;
			}

			return static_cast<MouseButton>(button);
		}

	}
}