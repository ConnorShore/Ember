#pragma once

#include "Ember/Core/Core.h"
#include "Ember/Math/Math.h"
#include "Ember/Render/VertexArray.h"
#include "Ember/Render/Shader.h"

namespace Ember {

	class RendererAPI
	{
	public:
		enum class API
		{
			None = 0,
			OpenGL
		};

		enum class RenderBit : uint32_t
		{
			Color = 1 << 0,
			Depth = 1 << 1
		};

		enum class Face
		{
			Front,
			Back
		};

		using RenderBits = uint32_t;

		virtual void Clear() = 0;
		virtual void Clear(RenderBits bits) = 0;
		void Clear(RenderBit bit) { Clear(static_cast<RenderBits>(bit)); }
		virtual void SetClearColor(Vector4<float> color) = 0;
		virtual void UseFaceCulling(bool use) = 0;
		virtual void CullFace(Face face) = 0;
		virtual void UseDepthTest(bool use) = 0;
		virtual void UseBlending(bool use) = 0;

		virtual void SetTextureUnit(unsigned int unit, unsigned int texture) = 0;
		virtual void SetFramebuffer(unsigned int framebufferId) = 0;
		virtual void GetPreviousFramebuffer(int* outFramebufferId) = 0;

		virtual void CopyDepthBuffer(unsigned int gBufferId, unsigned int outputBuffer, Vector4<int> viewportDims) = 0;

		virtual void GetViewportDimensions(int* outViewportDims) = 0;
		
		virtual void SetViewport(unsigned int x, unsigned int y, unsigned int width, unsigned int  height) = 0;

		virtual void DrawIndexed(const SharedPtr<VertexArray>& vertexArray) = 0;
		virtual void DrawIndexed(const SharedPtr<VertexArray>& vertexArray, unsigned int indicesCt) = 0;

	public:
		static API GetApi() { return s_Api; }
		static ScopedPtr<RendererAPI> Create();

	private:
		static API s_Api;
	};

	inline RendererAPI::RenderBits operator|(RendererAPI::RenderBit a, RendererAPI::RenderBit b)
	{
		return static_cast<RendererAPI::RenderBits>(a) | static_cast<RendererAPI::RenderBits>(b);
	}

	inline RendererAPI::RenderBits operator|(RendererAPI::RenderBits a, RendererAPI::RenderBit b)
	{
		return a | static_cast<RendererAPI::RenderBits>(b);
	}

}