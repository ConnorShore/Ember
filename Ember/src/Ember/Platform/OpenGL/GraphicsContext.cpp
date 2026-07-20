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

			bool gladLoaded = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
			EB_CORE_ASSERT(gladLoaded, "Failed to load glad!");
			EB_CORE_TRACE("------------- OpenGL Info -------------");
			EB_CORE_TRACE("Vendor:   {}", (const char*)glGetString(GL_VENDOR));
			EB_CORE_TRACE("Renderer: {}", (const char*)glGetString(GL_RENDERER));
			EB_CORE_TRACE("Version:  {}", (const char*)glGetString(GL_VERSION));
			EB_CORE_TRACE("---------------------------------------");
			
			// Enable synchronous GL error/UB callbacks in every build except the shipping Dist build.
			// Crucially this INCLUDES Release: the whole class of "works in Debug, breaks in Release"
			// GPU bugs only surfaces in optimized builds, so we want GL diagnostics there too.
			// Suppress NOTIFICATION severity to reduce noise. Relies on the debug context requested in
			// Windows::Window (GLFW_OPENGL_DEBUG_CONTEXT).
#ifndef EB_DIST
			glEnable(GL_DEBUG_OUTPUT);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
			glDebugMessageCallback(GLMessageCallback, nullptr);
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, false);
#endif
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
			case GL_DEBUG_SEVERITY_HIGH:			EB_CORE_ERROR("GL Callback [{}]: {}", _type, (const char*)msg); break;
			case GL_DEBUG_SEVERITY_MEDIUM:
			case GL_DEBUG_SEVERITY_LOW:				EB_CORE_WARN("GL Callback [{}]: {}", _type, (const char*)msg); break;
			case GL_DEBUG_SEVERITY_NOTIFICATION:
			default:								EB_CORE_INFO("GL Callback [{}]: {}", _type, (const char*)msg); break;
			}
		}

	}
}