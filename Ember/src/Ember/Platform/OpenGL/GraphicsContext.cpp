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

			EB_CORE_ASSERT(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress), "Failed to load glad!");
			EB_CORE_TRACE("------------- OpenGL Info -------------");
			EB_CORE_TRACE("Vendor:   {}", (const char*)glGetString(GL_VENDOR));
			EB_CORE_TRACE("Renderer: {}", (const char*)glGetString(GL_RENDERER));
			EB_CORE_TRACE("Version:  {}", (const char*)glGetString(GL_VERSION));
			EB_CORE_TRACE("---------------------------------------");
			
			// Enable error callbacks
			glEnable(GL_DEBUG_OUTPUT);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
			glDebugMessageCallback(GLMessageCallback, nullptr);
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, true);
		}

		void GraphicsContext::SwapBuffers()
		{
			glfwSwapBuffers(m_WindowHandle);
		}

		void APIENTRY GraphicsContext::GLMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* msg, const void* data)
		{
			const char* _type;
			switch (type) {
			case GL_DEBUG_TYPE_ERROR:				_type = "ERROR"; break;
			case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: _type = "DEPRECATED BEHAVIOR"; break;
			case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:	_type = "UDEFINED BEHAVIOR"; break;
			case GL_DEBUG_TYPE_PORTABILITY:			_type = "PORTABILITY"; break;
			case GL_DEBUG_TYPE_PERFORMANCE:			_type = "PERFORMANCE"; break;
			case GL_DEBUG_TYPE_OTHER:				_type = "OTHER"; break;
			case GL_DEBUG_TYPE_MARKER:				_type = "MARKER"; break;
			default:								_type = "UNKNOWN"; break;
			}

			switch (severity) {
			case GL_DEBUG_SEVERITY_HIGH:			EB_CORE_ERROR("[{}]: {}", _type, (const char*)msg); break;
			case GL_DEBUG_SEVERITY_MEDIUM:			EB_CORE_WARN("[{}]: {}", _type, (const char*)msg); break;
			case GL_DEBUG_SEVERITY_LOW:				EB_CORE_INFO("[{}]: {}", _type, (const char*)msg); break;
			case GL_DEBUG_SEVERITY_NOTIFICATION:
			default:								EB_CORE_TRACE("[{}]: {}", _type, (const char*)msg); break;
			}
		}

	}
}