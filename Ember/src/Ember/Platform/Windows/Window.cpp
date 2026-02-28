#include "ebpch.h"
#include "Window.h"

namespace Ember {

	Window* Window::Create(const WindowConfig& config)
	{
		return new Windows::Window(config);
	}

	namespace Windows {

		Ember::Windows::Window::Window(const WindowConfig& config)
			: m_Width(config.Width), m_Height(config.Height), m_Title(config.Title)
		{
			EB_CORE_ASSERT(glfwInit(), "Failed to initalize GLFW!");

			m_Window = ScopedPtr<GLFWwindow>::Create(glfwCreateWindow(config.Width, config.Height, config.Title.c_str(), NULL, NULL));
			if (!m_Window)
			{
				glfwTerminate();
				EB_CORE_ERROR("Failed to create GLFW window!");
				return;
			}

			glfwMakeContextCurrent(m_Window.Ptr());
		}

		Ember::Windows::Window::~Window()
		{
			glfwTerminate();
		}

		void Ember::Windows::Window::OnUpdate()
		{

		}

	}
}