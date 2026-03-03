#pragma once

#include "Ember/Render/GraphicsContext.h"

#include <glad/glad.h>

class GLFWwindow;

namespace Ember {
	namespace OpenGL {

		class GraphicsContext : public Ember::GraphicsContext
		{
		public:
			GraphicsContext(GLFWwindow* window);
			virtual ~GraphicsContext();

			virtual void Init() override;
			virtual void SwapBuffers() override;

		private:
			static void APIENTRY GLMessageCallback(GLenum source, GLenum type, GLuint id,
				GLenum severity, GLsizei length,
				const GLchar* msg, const void* data);

		private:
			GLFWwindow* m_WindowHandle;
		};

	}
}