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

		virtual void Clear() = 0;
		virtual void SetClearColor(Vector4<float> color) = 0;
		virtual void UseFaceCulling(bool use) = 0;
		virtual void UseDepthTest(bool use) = 0;
		virtual void UseBlending(bool use) = 0;

		virtual void DrawIndexed(const SharedPtr<VertexArray>& vertexArray) = 0;
		virtual void DrawIndexed(const SharedPtr<VertexArray>& vertexArray, unsigned int indicesCt) = 0;

	public:
		static API GetApi() { return s_Api; }
		static ScopedPtr<RendererAPI> Create();

	private:
		static API s_Api;
	};

}