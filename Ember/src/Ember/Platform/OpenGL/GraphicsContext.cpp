#include "ebpch.h"
#include "GraphicsContext.h"
#include "Ember/Core/Core.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace Ember {
	namespace OpenGL {

		GraphicsContext::GraphicsContext(GLFWwindow* window)
			: m_WindowHandle(window)
		{
		}

		GraphicsContext::~GraphicsContext()
		{
		}

		void GraphicsContext::Init()
		{
			glfwMakeContextCurrent(m_WindowHandle);

			EB_CORE_ASSERT(gladLoadGL(), "Failed to load glad!");
		}

		void GraphicsContext::SwapBuffers()
		{
			glfwSwapBuffers(m_WindowHandle);
		}

	}
}