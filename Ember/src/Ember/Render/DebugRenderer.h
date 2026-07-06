#pragma once

#include "Ember/Math/Math.h"

#include <vector>

namespace Ember {

	struct DebugVertex
	{
		Vector3f Position;
		Vector4f Color;
	};

	class DebugRenderer
	{
	public:
		static void DrawLine(const Vector3f& pointA, const Vector3f& pointB, const Vector4f& color = Vector4f(1.0f));
		static void DrawLine(const Vector3f& pointA, const Vector3f& direction, float length, const Vector4f& color = Vector4f(1.0f));
		static void DrawTriangle(const Vector3f& pointA, const Vector3f& pointB, const Vector3f& pointC, const Vector4f& color);
		static void DrawFilledTriangle(const Vector3f& pointA, const Vector3f& pointB, const Vector3f& pointC, const Vector4f& color);
		static void DrawCube(const Vector3f& center, const Vector3f& scale, const Vector4f& color);
		static void DrawOctahedron(const Vector3f& center, float size, const Vector4f& color);

		// Added getter for the RenderSystem
		static const std::vector<DebugVertex>& GetVertices();
		static const std::vector<DebugVertex>& GetFilledTriangleVertices();
		static void Clear();

	private:
		inline static std::vector<DebugVertex> s_Vertices;
		inline static std::vector<DebugVertex> s_FilledTriangleVertices;
	};

}