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
		static void SetBillboardCameraData(const Vector3f& cameraPosition, const Vector3f& cameraRight, const Vector3f& cameraUp);

		static void StartBatch();
		static void FlushBatch();
		static void NextBatch();

		static void DrawQuad(const Vector2f& position, const Vector2f& size, const Vector4f& color);
		static void DrawQuad(const Matrix4f& transform, const Vector4f& color);
		static void DrawQuad(const Vector2f& position, const Vector2f& size, const Vector4f& color, const SharedPtr<Texture2D>& texture);
		static void DrawQuad(const Matrix4f& transform, const Vector4f& color, const SharedPtr<Texture2D>& texture);
		static void DrawQuad(const Matrix4f& transform, const Vector4f& color, const SharedPtr<Texture2D>& texture, const Vector2f* customTexCoords, EntityID entity);
		static void DrawBillboardQuad(const Vector3f& center, const Vector2f& size, const Vector4f& color, bool lockYAxis = false);
		static void DrawBillboardQuad(const Vector3f& center, const Vector2f& size, const Vector4f& color, const SharedPtr<Texture2D>& texture, bool lockYAxis = false);

		static void DrawString(const std::string& text, const Matrix4f& transform, const Vector4f& color, const SharedPtr<Font>& font, EntityID entity, bool isScreenSpace = false);

		// Emits a quad as nine pieces so the corners keep their authored pixel size while the edges
		// and centre stretch. `border` is (left, bottom, right, top) in source-texture pixels.
		static void DrawNineSliceQuad(const Matrix4f& transform, const Vector4f& color,
			const SharedPtr<Texture2D>& texture, const Vector4f& border, EntityID entity);

		// Size the font atlas is baked at; screen-space glyph quads come out in these pixel units.
		static constexpr float FontBakePixelHeight = 32.0f;

		// Measured extents of a string in bake-pixel units, relative to the DrawString origin.
		// Shared so text alignment and hit-testing agree on where a label actually sits.
		static bool MeasureString(const std::string& text, const SharedPtr<Font>& font, Vector2f& outMin, Vector2f& outMax);
	};

}