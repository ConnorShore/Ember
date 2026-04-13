#include "ebpch.h"
#include "Window.h"
#include "Ember/Core/Core.h"
#include "Input.h"
#include "Ember/Event/WindowEvent.h"
#include "Ember/Event/KeyEvent.h"
#include "Ember/Event/MouseEvent.h"
#include "Ember/Input/Input.h"

#include <glad/glad.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <dwmapi.h>

// Link against the DWM library (for DwmSetWindowAttribute)
#pragma comment(lib, "dwmapi.lib")

namespace Ember {
	namespace Windows {

		static bool s_GLFWInitialized = false;

		Window::Window(const WindowConfig& config)
			: m_WindowData({ config.Title, config.Width, config.Height })
		{
			EB_CORE_INFO("Creating Windows (GLFW) window: {0} ({1}x{2})", config.Title, config.Width, config.Height);

			if (!s_GLFWInitialized)
			{
				EB_CORE_INFO("Initializing GLFW...");
				EB_CORE_ASSERT(glfwInit(), "Failed to initalize GLFW!");
				s_GLFWInitialized = true;
			}
			// Hide the window before creation to prevent the "teleport flicker"
			glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

			m_Window = glfwCreateWindow(config.Width, config.Height, config.Title.c_str(), NULL, NULL);
			if (!m_Window)
			{
				glfwTerminate();
				EB_CORE_ERROR("Failed to create GLFW window!");
				return;
			}

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

			// Reveal the window now that it is placed
			glfwShowWindow(m_Window);

			// Set dark theme for the window (Windows 10/11)
			HWND hwnd = glfwGetWin32Window(m_Window);
			BOOL useDarkMode = TRUE;
			DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));

			// Create graphics context
			m_GraphicsContext = GraphicsContext::Create(m_Window);
			m_GraphicsContext->Init();

			// Store our WindowData struct in GLFW's user pointer so lambdas can access it
			glfwSetWindowUserPointer(m_Window, &m_WindowData);

			//SetVSync(true);

			RegisterCallbacks();
		}

		Window::~Window()
		{
			glfwDestroyWindow(m_Window);
			glfwTerminate();

			EB_CORE_INFO("GLFW window destroyed!");
		}

		void Window::OnUpdate()
		{
			glfwPollEvents();
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
				});

			// Mouse Callbacks
			glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* w, int button, int action, int mods)
				{
					WindowData& data = *(WindowData*)glfwGetWindowUserPointer(w);
					MouseButton mouseButton = Input::GlfwMouseButtonToEmberMouseButton(button);

					switch (action)
					{
					case GLFW_PRESS:
					{
						MousePressedEvent e(mouseButton);
						data.EventCallback(e);
						break;
					}
					case GLFW_RELEASE:
					{
						MouseReleasedEvent e(mouseButton);
						data.EventCallback(e);
						break;
					}
					}
				});

			// Mouse Move callback
			glfwSetCursorPosCallback(m_Window, [](GLFWwindow* w, double xpos, double ypos)
				{
					WindowData& data = *(WindowData*)glfwGetWindowUserPointer(w);
					MouseMovedEvent e(Vector2f((float)xpos, (float)ypos));
					data.EventCallback(e);
				});

			// Mouse Scroll callback
			glfwSetScrollCallback(m_Window, [](GLFWwindow* w, double xoffset, double yoffset)
				{
					WindowData& data = *(WindowData*)glfwGetWindowUserPointer(w);
					MouseScrolledEvent e(Vector2f((float)xoffset, (float)yoffset));
					data.EventCallback(e);
				});
		}

	}
}