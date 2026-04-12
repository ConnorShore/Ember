#pragma once

#include "Ember/Math/Math.h"

#include <vector>

namespace Ember {

	// Must match the layout of rp3d::DebugRenderer::DebugLine for easy conversion
	//struct DebugLine
	//{
	//	Vector3f Point1;
	//	uint32_t Color1;
	//	Vector3f Point2;
	//	uint32_t Color2;
	//};

	//struct DebugTriangle
	//{
	//	Vector3f Point1;
	//	uint32_t Color1;
	//	Vector3f Point2;
	//	uint32_t Color2;
	//	Vector3f Point3;
	//	uint32_t Color3;
	//};

	struct DebugVertex
	{
		Vector3f Position;
		Vector4f Color;
	};

	class DebugRenderer
	{
	public:
		static void DrawLine(const Vector3f& pointA, const Vector3f& pointB, const Vector4f& color);
		static void DrawTriangle(const Vector3f& pointA, const Vector3f& pointB, const Vector3f& pointC, const Vector4f& color);

		// Added getter for the RenderSystem
		static const std::vector<DebugVertex>& GetVertices();
		static void Clear();

	private:
		inline static std::vector<DebugVertex> s_Vertices;
	};

}