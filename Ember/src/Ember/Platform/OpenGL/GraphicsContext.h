#pragma once

#include "Ember/Render/GraphicsContext.h"

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
			GLFWwindow* m_WindowHandle;
		};

	}
}