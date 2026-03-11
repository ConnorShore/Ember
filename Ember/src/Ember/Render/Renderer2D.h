#pragma once

#include "Camera.h"
#include "Texture.h"
#include "Ember/ECS/Component/Components.h"

namespace Ember {

	class Renderer2D
	{
	public:
		static void Init();
		static void Shutdown();

		static void BeginFrame(CameraComponent& camera, const Matrix4f& transform);
		static void EndFrame();

		static void StartBatch();
		static void FlushBatch();
		static void NextBatch();

		static void DrawQuad(const Vector2f& position, const Vector2f& size, const Vector4f& color);
		static void DrawQuad(const Matrix4f& transform, const Vector4f& color);
		static void DrawQuad(const Vector2f& position, const Vector2f& size, const Vector4f& color, const SharedPtr<Texture>& texture);
		static void DrawQuad(const Matrix4f& transform, const Vector4f& color, const SharedPtr<Texture>& texture);
	};

}