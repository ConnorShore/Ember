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

		enum class DepthFunction
		{
			Less,
			LessEqual,
			Greater,
			GreaterEqual,
			Equal,
			NotEqual,
			Always,
			Never
		};

		using RenderBits = uint32_t;

		virtual void Clear() = 0;
		virtual void Clear(RenderBits bits) = 0;
		void Clear(RenderBit bit) { Clear(static_cast<RenderBits>(bit)); }
		virtual void SetClearColor(Vector4<float> color) = 0;
		virtual void UseFaceCulling(bool use) = 0;
		virtual void CullFace(Face face) = 0;
		virtual void UseDepthTest(bool use) = 0;
		virtual void UseDepthMask(bool use) = 0;
		virtual void UseBlending(bool use) = 0;
		virtual void UseCubeMapSeamless(bool use) = 0;

		virtual void UseDepthFunction(DepthFunction func) = 0;

		virtual void SetTextureUnit(uint32_t unit, uint32_t texture) = 0;
		virtual void SetFramebuffer(uint32_t framebufferId) = 0;
		virtual void GetPreviousFramebuffer(int* outFramebufferId) = 0;

		virtual void CopyDepthBuffer(uint32_t gBufferId, uint32_t outputBuffer, Vector4<int> viewportDims) = 0;

		virtual void GetViewportDimensions(int* outViewportDims) = 0;
		
		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;

		virtual void DrawIndexed(const SharedPtr<VertexArray>& vertexArray) = 0;
		virtual void DrawIndexed(const SharedPtr<VertexArray>& vertexArray, uint32_t indicesCt) = 0;

		virtual void DrawLines(const SharedPtr<VertexArray>& vertexArray, uint32_t vertexCount) = 0;

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