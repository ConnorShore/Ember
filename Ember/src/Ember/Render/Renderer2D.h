#pragma once

#include "Camera.h"
#include "Texture2D.h"
#include "Ember/ECS/Component/Components.h"
#include "Ember/Asset/Font.h"

namespace Ember {

	class Renderer2D
	{
	public:
		static void Init();
		static void Shutdown();

		static void BeginFrame();
		static void EndFrame();

		static void StartBatch();
		static void FlushBatch();
		static void NextBatch();

		static void DrawQuad(const Vector2f& position, const Vector2f& size, const Vector4f& color);
		static void DrawQuad(const Matrix4f& transform, const Vector4f& color);
		static void DrawQuad(const Vector2f& position, const Vector2f& size, const Vector4f& color, const SharedPtr<Texture2D>& texture);
		static void DrawQuad(const Matrix4f& transform, const Vector4f& color, const SharedPtr<Texture2D>& texture);
		static void DrawQuad(const Matrix4f& transform, const Vector4f& color, const SharedPtr<Texture2D>& texture, const Vector2f* customTexCoords);

		static void DrawString(const std::string& text, const Matrix4f& transform, const Vector4f& color, const SharedPtr<Font>& font);
	};

}