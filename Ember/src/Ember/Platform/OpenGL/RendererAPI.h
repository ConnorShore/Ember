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
			virtual void CullFace(Face face) override;
			virtual void UseDepthTest(bool use) override;
			virtual void UseDepthMask(bool use) override;
			virtual void UseBlending(bool use) override;
			virtual void UseCubeMapSeamless(bool use) override;

			virtual void UseDepthFunction(DepthFunction func) override;

			virtual void SetTextureUnit(uint32_t unit, uint32_t texture) override;
			virtual void SetFramebuffer(uint32_t framebufferId) override;
			virtual void GetPreviousFramebuffer(int* outFramebufferId) override;


			virtual void CopyDepthBuffer(uint32_t gBufferId, uint32_t outputBuffer, Vector4<int> viewportDims) override;

			virtual void GetViewportDimensions(int* outViewportDims) override;

			virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
			
			virtual void DrawIndexed(const SharedPtr<VertexArray>& vertexArray) override;
			virtual void DrawIndexed(const SharedPtr<VertexArray>& vertexArray, uint32_t indicesCt) override;

			virtual void DrawIndexedInstanced(const SharedPtr<VertexArray>& vertexArray, uint32_t indexCount, uint32_t instanceCount) override;

			virtual void DrawLines(const SharedPtr<VertexArray>& vertexArray, uint32_t vertexCount) override;
		};

	}
}