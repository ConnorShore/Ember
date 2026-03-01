#include "ebpch.h"
#include "Input.h"

#include "Ember/Core/Core.h"

#include <GLFW/glfw3.h>

namespace Ember {
	namespace Windows {

		/// <summary>
		/// Maps GLFW key codes to Ember key codes
		/// 
		/// Currently the Ember key codes match the GLFW key codes so no conversion
		/// is necessary
		/// </summary>
		/// <param name="key">GLFW key code</param>
		/// <returns>Ember key code</returns>
		KeyCode Input::GlfwKeyCodeToEmberKeyCode(int key)
		{
			if (key < 0 || KeyCode::Last < key)
			{
				EB_CORE_ASSERT("Undefined key code: {0}", key);
				return KeyCode::Unknown;
			}

			return static_cast<KeyCode>(key);
		}

		/// <summary>
		/// Maps GLFW mouse button codes to Ember mouse button codes
		/// 
		/// Currently the Ember mouse button codes match the GLFW mouse button codes so no conversion
		/// is necessary
		/// </summary>
		/// <param name="key">GLFW mouse button code</param>
		/// <returns>Ember mouse button code</returns>
		Ember::MouseButton Input::GlfwMouseButtonToEmberMouseButton(int button)
		{
			if (MouseButton::Last < button)
			{
				EB_CORE_ASSERT("Undefined mouse button code: {0}", button);
				return MouseButton::Unknown;
			}
		}

	}
}