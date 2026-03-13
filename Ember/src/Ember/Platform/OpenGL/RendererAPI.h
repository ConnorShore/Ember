#pragma once

#include "Ember/Render/RendererAPI.h"

namespace Ember {
	namespace OpenGL {

		class RendererAPI : public Ember::RendererAPI
		{
		public:
			virtual void Clear() override;
			virtual void Clear(RenderBits bits) override;
			virtual void SetClearColor(Vector4<float> color) override;
			virtual void UseFaceCulling(bool use) override;
			virtual void UseDepthTest(bool use) override;
			virtual void UseBlending(bool use) override;
			virtual void SetViewport(unsigned int x, unsigned int y, unsigned int width, unsigned int  height) override;
			virtual void DrawIndexed(const SharedPtr<VertexArray>& vertexArray) override;
			virtual void DrawIndexed(const SharedPtr<VertexArray>& vertexArray, unsigned int indicesCt) override;
		};

	}
}