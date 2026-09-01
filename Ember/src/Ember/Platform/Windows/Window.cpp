#include "ebpch.h"
#include "Window.h"
#include "Ember/Core/Core.h"
#include "Ember/Core/Paths.h"
#include "Input.h"
#include "Ember/Event/WindowEvent.h"
#include "Ember/Event/KeyEvent.h"
#include "Ember/Event/MouseEvent.h"
#include "Ember/Input/Input.h"

#include <glad/glad.h>
#include <stb_image.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <dwmapi.h>
#include <stdexcept>

// Link against the DWM library (for DwmSetWindowAttribute)
#pragma comment(lib, "dwmapi.lib")

namespace Ember {
	namespace Windows {

		static void GLFWErrorCallback(int errorCode, const char* description)
		{
			EB_CORE_ERROR("GLFW Error [{}]: {}", errorCode, description ? description : "No description provided");
		}

		static bool s_GLFWInitialized = false;

		// Layered over GLFW's built-in table so a pad newer than the vendored SDL_GameControllerDB
		// snapshot can be enabled without patching the submodule.
		static std::filesystem::path GamepadMappingsPath()
		{
			return Paths::EngineAssets() / "input/gamecontrollerdb.txt";
		}

		static void LogJoystick(int jid)
		{
			const char* guid = glfwGetJoystickGUID(jid);
			const char* name = glfwGetJoystickName(jid);

			if (glfwJoystickIsGamepad(jid))
			{
				const char* mapped = glfwGetGamepadName(jid);
				EB_CORE_INFO("Gamepad {} connected: '{}' [{}]", jid, mapped ? mapped : "unknown", guid ? guid : "no GUID");
				return;
			}

			// PollGamepadStates drops unmapped pads, so name the GUID a mapping line would have to match.
			EB_CORE_WARN("Joystick {} connected but GLFW has no gamepad mapping for it: '{}' [{}]. Add an "
				"SDL_GameControllerDB line for that GUID to {} to enable it.",
				jid, name ? name : "unknown", guid ? guid : "no GUID", GamepadMappingsPath().string());
		}

		static void JoystickCallback(int jid, int event)
		{
			if (event == GLFW_CONNECTED)
			{
				LogJoystick(jid);
				return;
			}

			EB_CORE_INFO("Joystick {} disconnected.", jid);
		}

		static void InitGamepadSupport()
		{
			const std::filesystem::path mappingsPath = GamepadMappingsPath();
			if (std::filesystem::exists(mappingsPath))
			{
				std::ifstream file(mappingsPath);
				std::stringstream mappings;
				mappings << file.rdbuf();

				if (glfwUpdateGamepadMappings(mappings.str().c_str()))
				{
					EB_CORE_INFO("Loaded extra gamepad mappings from {}", mappingsPath.string());
				}
				else
				{
					EB_CORE_WARN("Failed to parse gamepad mappings at {}", mappingsPath.string());
				}
			}

			glfwSetJoystickCallback(JoystickCallback);

			// The callback only fires on hotplug, so anything already attached has to be reported here.
			for (int jid = GLFW_JOYSTICK_1; jid <= GLFW_JOYSTICK_LAST; ++jid)
			{
				if (glfwJoystickPresent(jid))
				{
					LogJoystick(jid);
				}
			}
		}

		Window::Window(const WindowConfig& config)
			: m_WindowData({ config.Title, config.Width, config.Height })
		{
			EB_CORE_INFO("Creating Windows (GLFW) window: {0} ({1}x{2})", config.Title, config.Width, config.Height);

			if (!s_GLFWInitialized)
			{
				EB_CORE_INFO("Initializing GLFW...");
				const int glfwInitResult = glfwInit();
				EB_CORE_ASSERT(glfwInitResult, "Failed to initalize GLFW!");

				if (!glfwInitResult)
				{
					const char* glfwErrorDescription = nullptr;
					const int glfwErrorCode = glfwGetError(&glfwErrorDescription);
					EB_CORE_ERROR("Failed to initialize GLFW. Error [{}]: {}", glfwErrorCode, glfwErrorDescription ? glfwErrorDescription : "No description provided");
					throw std::runtime_error("Failed to initialize GLFW");
				}

				glfwSetErrorCallback(GLFWErrorCallback);
				InitGamepadSupport();
				s_GLFWInitialized = true;
			}

			glfwDefaultWindowHints();
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifndef EB_DIST
			// Request a debug context so glDebugMessageCallback (GraphicsContext::Init) reliably
			// reports GL errors/undefined behavior. Enabled in Debug AND Release, off in shipping Dist.
			glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif
			// Hide the window before creation to prevent the "teleport flicker"
			glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

			// Create the GLFW window
			m_Window = glfwCreateWindow(config.Width, config.Height, config.Title.c_str(), NULL, NULL);
			if (!m_Window)
			{
				const char* glfwErrorDescription = nullptr;
				const int glfwErrorCode = glfwGetError(&glfwErrorDescription);
				EB_CORE_ERROR("Failed to create GLFW window. Error [{}]: {}", glfwErrorCode, glfwErrorDescription ? glfwErrorDescription : "No description provided");
				glfwTerminate();
				s_GLFWInitialized = false;
				EB_CORE_ERROR("Failed to create GLFW window!");
				throw std::runtime_error("Failed to create GLFW window");
			}

			// Set the window icon. Resolved here rather than as a WindowConfig default so that merely
			// declaring an ApplicationSpecification does not probe the filesystem.
			const std::filesystem::path iconPath = config.IconPath.empty()
				? Paths::EngineAssets() / "images/EmberIcon.png"
				: config.IconPath;

			if (!iconPath.empty())
			{
				GLFWimage icon;
				icon.pixels = stbi_load(iconPath.string().c_str(), &icon.width, &icon.height, 0, 4);
				if (icon.pixels)
				{
					glfwSetWindowIcon(m_Window, 1, &icon);
					stbi_image_free(icon.pixels);
				}
				else
				{
					EB_CORE_WARN("Failed to load window icon from path: {}", iconPath.string());
				}
			}

			// Set dark theme for the window (Windows 10/11)
			HWND hwnd = glfwGetWin32Window(m_Window);
			BOOL useDarkMode = TRUE;
			DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));

			// Center window on monitor
			GLFWmonitor* monitor = glfwGetPrimaryMonitor();
			const GLFWvidmode* mode = glfwGetVideoMode(monitor);
			if (mode)
			{
				int xPos = (mode->width - config.Width) / 2;
				// Offset slightly upwards (e.g., -30) to account for the Windows task bar feeling visually heavy
				int yPos = (mode->height - config.Height) / 2 - 30;

				glfwSetWindowPos(m_Window, xPos, yPos);
			}

			if (config.StartMaximized)
				glfwMaximizeWindow(m_Window);

			// Reveal the window now that it is placed
			glfwShowWindow(m_Window);

			// Create graphics context
			m_GraphicsContext = GraphicsContext::Create(m_Window);
			m_GraphicsContext->Init();

			// Store our WindowData struct in GLFW's user pointer so lambdas can access it
			glfwSetWindowUserPointer(m_Window, &m_WindowData);

			SetVSync(false);

			RegisterCallbacks();
		}

		Window::~Window()
		{
			glfwDestroyWindow(m_Window);
			glfwTerminate();

			EB_CORE_INFO("GLFW window destroyed!");
		}

		void Window::PollEvents()
		{
			EB_PROFILE_SCOPE("Window::PollEvents");
			glfwPollEvents();
			PollGamepadStates();
		}

		void Window::Present()
		{
			// Blocks here until vsync if enabled — a large/variable duration here usually means
			// GPU-bound or vsync-bound, not CPU-bound; compare against the "Frame" total.
			EB_PROFILE_SCOPE("Window::SwapBuffers");
			m_GraphicsContext->SwapBuffers();
		}

		void Window::SetVSync(bool enabled)
		{
			m_WindowData.VSync = enabled;
			glfwSwapInterval(enabled ? 1 : 0);
		}

		bool Window::IsVSync() const
		{
			return m_WindowData.VSync;
		}

		void Window::SetCursorPosition(float x, float y)
		{
			glfwSetCursorPos(m_Window, x, y);
		}

		void Window::SetCursorMode(CursorMode mode)
		{
			m_CursorMode = mode;

			switch (mode)
			{
			case CursorMode::Normal:
				glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				break;
			case CursorMode::Hidden:
				glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
				break;
			case CursorMode::Locked:
				glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				break;
			}
		}
		
		// Hook GLFW callbacks that translate native events into Ember events
		void Window::RegisterCallbacks()
		{
			glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* w)
				{
					WindowData& data = *(WindowData*)glfwGetWindowUserPointer(w);

					WindowCloseEvent e;
					data.EventCallback(e);
				});
			glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* w, int width, int height)
				{
					WindowData& data = *(WindowData*)glfwGetWindowUserPointer(w);
					data.Width = width;
					data.Height = height;

					WindowResizeEvent e(width, height);
					data.EventCallback(e);
				});

			// Clear cached input on focus loss. GLFW stops delivering key events while unfocused, so a key
			// released during that time would otherwise stay "stuck" pressed when focus returns.
			glfwSetWindowFocusCallback(m_Window, [](GLFWwindow* w, int focused)
				{
					if (!focused)
						Ember::Input::ClearAllStates();
				});

			// Key Callbacks
			glfwSetKeyCallback(m_Window, [](GLFWwindow* w, int key, int scancode, int action, int mods)
				{
					WindowData& data = *(WindowData*)glfwGetWindowUserPointer(w);
					KeyCode keyCode = Input::GlfwKeyCodeToEmberKeyCode(key);

					switch (action)
					{
					case GLFW_PRESS:
					{
						KeyPressedEvent e(keyCode);
						data.EventCallback(e);
						break;
					}
					case GLFW_REPEAT:
					{
						KeyRepeatEvent e(keyCode);
						data.EventCallback(e);
						break;
					}
					case GLFW_RELEASE:
					{
						KeyReleasedEvent e(keyCode);
						data.EventCallback(e);
						break;
					}
					}

					Ember::Input::SetLastUsedInputDevice(InputDevice::Keyboard);
				});

			// Mouse Callbacks
			glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* w, int button, int action, int mods)
				{
					WindowData& data = *(WindowData*)glfwGetWindowUserPointer(w);
					MouseControl mouseControl{};
					if (!Input::GlfwMouseButtonToEmberMouseControl(button, mouseControl))
						return;

					switch (action)
					{
					case GLFW_PRESS:
					{
						MousePressedEvent e(mouseControl);
						data.EventCallback(e);
						break;
					}
					case GLFW_RELEASE:
					{
						MouseReleasedEvent e(mouseControl);
						data.EventCallback(e);
						break;
					}
					}

					Ember::Input::SetLastUsedInputDevice(InputDevice::Mouse);
				});

			// Mouse Move callback
			glfwSetCursorPosCallback(m_Window, [](GLFWwindow* w, double xpos, double ypos)
				{
					WindowData& data = *(WindowData*)glfwGetWindowUserPointer(w);
					MouseMovedEvent e(Vector2f((float)xpos, (float)ypos));
					data.EventCallback(e);

					Ember::Input::SetLastUsedInputDevice(InputDevice::Mouse);
				});

			// Mouse Scroll callback
			glfwSetScrollCallback(m_Window, [](GLFWwindow* w, double xoffset, double yoffset)
				{
					WindowData& data = *(WindowData*)glfwGetWindowUserPointer(w);
					MouseScrolledEvent e(Vector2f((float)xoffset, (float)yoffset));
					data.EventCallback(e);

					Ember::Input::SetLastUsedInputDevice(InputDevice::Mouse);
				});

		}

		// A stick is deadzoned on its combined magnitude and rescaled so the value still ramps from zero
		// at the edge of the deadzone; a per-axis floor would notch the diagonals.
		static bool ApplyStickDeadzone(GamepadState& state, GamepadAxis xAxis, GamepadAxis yAxis)
		{
			const size_t x = static_cast<size_t>(xAxis);
			const size_t y = static_cast<size_t>(yAxis);

			const Vector2f stick = { state.Axis[x], state.Axis[y] };
			const float magnitude = Math::Length(stick);
			const float deadzone = Ember::Input::GamepadStickDeadzone;

			if (magnitude < deadzone)
			{
				state.Axis[x] = 0.0f;
				state.Axis[y] = 0.0f;
				return false;
			}

			const float rescaled = Math::Clamp((magnitude - deadzone) / (1.0f - deadzone), 0.0f, 1.0f);
			state.Axis[x] = stick.x * (rescaled / magnitude);
			state.Axis[y] = stick.y * (rescaled / magnitude);
			return true;
		}

		// GLFW rests a trigger at -1, so remap onto the [0,1] the rest of the engine expects.
		static bool ApplyTriggerRange(GamepadState& state, GamepadAxis axis)
		{
			const size_t index = static_cast<size_t>(axis);
			const float value = (state.Axis[index] + 1.0f) * 0.5f;
			state.Axis[index] = value < Ember::Input::GamepadTriggerDeadzone ? 0.0f : value;
			return value >= Ember::Input::GamepadTriggerDeadzone;
		}

		void Window::PollGamepadStates()
		{
			bool anyUsed = false;
			for (size_t i = 0; i < Ember::Input::MaxGamepads; ++i)
			{
				GamepadState& state = Ember::Input::GetGamepadState(i);
				const int joystick = static_cast<int>(i);

				state.PreviousDown = state.Down;

				GLFWgamepadstate glfwState;
				const bool connected = glfwJoystickPresent(joystick)
					&& glfwJoystickIsGamepad(joystick)
					&& glfwGetGamepadState(joystick, &glfwState);

				// Clearing on loss matters: a pad unplugged mid-hold would otherwise stay latched down.
				if (!connected)
				{
					state.Connected = false;
					state.Down = 0;
					state.Axis.fill(0.0f);
					continue;
				}

				state.Connected = true;
				state.Down = 0;

				// GLFW's _LAST codes name the final control, unlike Ember's one-past-the-end Last sentinels.
				for (int button = 0; button <= GLFW_GAMEPAD_BUTTON_LAST; ++button)
				{
					if (glfwState.buttons[button] != GLFW_PRESS)
					{
						continue;
					}

					GamepadButton control{};
					if (Input::GlfwGamepadButtonToEmberGamepadControl(button, control))
					{
						state.Down |= (1 << static_cast<GamepadButtonType>(control));
					}

					anyUsed = true;
				}

				for (int axis = 0; axis <= GLFW_GAMEPAD_AXIS_LAST; ++axis)
				{
					GamepadAxis control{};
					if (Input::GlfwGamepadAxisToEmberGamepadControl(axis, control))
					{
						state.Axis[static_cast<size_t>(control)] = glfwState.axes[axis];
					}
				}

				// Filtering runs after the raw copy because a stick is deadzoned as a pair, not per axis.
				anyUsed |= ApplyStickDeadzone(state, GamepadAxis::LeftX, GamepadAxis::LeftY);
				anyUsed |= ApplyStickDeadzone(state, GamepadAxis::RightX, GamepadAxis::RightY);
				anyUsed |= ApplyTriggerRange(state, GamepadAxis::LeftTrigger);
				anyUsed |= ApplyTriggerRange(state, GamepadAxis::RightTrigger);
			}

			if (anyUsed)
				Ember::Input::SetLastUsedInputDevice(InputDevice::Gamepad);
		}
	}
}