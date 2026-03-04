#pragma once

#include "Ember/Core/Core.h"
#include "Ember/Math/Math.h"
#include "RendererAPI.h"
#include "VertexArray.h"
#include "Shader.h"
#include "Camera.h"

namespace Ember {

	class Renderer
	{
	public:
		static void BeginFrame(Camera& camera);
		static void EndFrame();
		static void Submit(const SharedPtr<VertexArray>& vertexArray, const SharedPtr<Shader>& shader);

		static RendererAPI::API GetApi() { return RendererAPI::GetApi(); }

	private:
		struct FrameData {
			Matrix4f ViewProjectionMatrix;
		};

		static ScopedPtr<FrameData> s_FrameData;
	};

}