#include "ebpch.h"
#include "Window.h"

namespace Ember {

	Window* Window::Create(const WindowConfig& config)
	{
		return new Windows::Window(config);
	}

	static bool s_GLFWInitialized = false;

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