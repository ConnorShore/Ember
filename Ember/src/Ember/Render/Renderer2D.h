#pragma once

#include "Camera.h"

namespace Ember {

	class Renderer2D
	{
	public:
		static void Init();
		static void Shutdown();

		static void StartBatch();
		static void FlushBatch();
		static void NextBatch();

		static void BeginFrame(OrthographicCamera& camera);
		static void EndFrame();

		static void DrawQuad(const Vector2f& position, const Vector2f& size, const Vector4f& color);
		static void DrawQuad(const Matrix4f& transform, const Vector4f& color);
	};

}