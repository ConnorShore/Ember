#include "ebpch.h"
#include "RendererAPI.h"

#include <glad/glad.h>

namespace Ember {
	namespace OpenGL {

		void RendererAPI::Clear()
		{
			glClear(GL_COLOR_BUFFER_BIT);
		}

		void RendererAPI::SetClearColor(Vector4<float> color)
		{
			glClearColor(color[0], color[1], color[2], color[3]);
		}

		void RendererAPI::UseFaceCulling(bool use)
		{
			use ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
		}

		void RendererAPI::UseDepthTest(bool use)
		{
			use ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
		}

	}
}