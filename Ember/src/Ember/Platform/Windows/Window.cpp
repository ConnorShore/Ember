#include "ebpch.h"
#include "Window.h"
#include "Input.h"
#include "Ember/Event/WindowEvent.h"
#include "Ember/Event/KeyEvent.h"
#include "Ember/Event/MouseEvent.h"
#include "Ember/Input/Input.h"

namespace Ember {

	//------- Static Methods/Members --------------------------------

	Window* Window::Create(const WindowConfig& config)
	{
		return new Windows::Window(config);
	}

	static bool s_GLFWInitialized = false;

	//---------------------------------------------------------------

	namespace Windows {

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

			m_Window = glfwCreateWindow(config.Width, config.Height, config.Title.c_str(), NULL, NULL);
			if (!m_Window)
			{
				glfwTerminate();
				EB_CORE_ERROR("Failed to create GLFW window!");
				return;
			}

			glfwMakeContextCurrent(m_Window);
			glfwSetWindowUserPointer(m_Window, &m_WindowData);

			// Set GLFW callbacks
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
		}

		Window::~Window()
		{
			glfwDestroyWindow(m_Window);
			glfwTerminate();

			EB_CORE_INFO("GLFW window destroyed!");
		}

		void Window::OnUpdate()
		{
			glClear(GL_COLOR_BUFFER_BIT);
			glfwSwapBuffers(m_Window);
			glfwPollEvents();
		}
	}
}