#pragma once

#include "Ember/Core/Core.h"
#include "Ember/Math/Math.h"
#include "RendererAPI.h"
#include "VertexArray.h"
#include "Shader.h"
#include "Camera.h"
#include "Ember/ECS/Component/Components.h"

namespace Ember {

	class Renderer
	{
	public:
		static void BeginFrame(Camera& camera);
		static void EndFrame();
		static void Submit(const SharedPtr<VertexArray>& vertexArray, const SharedPtr<Shader>& shader);

		//static void DrawSprite(const SpriteComponent& sprite, const Matrix4f transform);

		static RendererAPI::API GetApi() { return RendererAPI::GetApi(); }

	private:
		struct FrameData {
			Matrix4f ViewProjectionMatrix;
		};

		static ScopedPtr<FrameData> s_FrameData;
	};

}