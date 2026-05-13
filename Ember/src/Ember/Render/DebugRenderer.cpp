#include "ebpch.h"
#include "DebugRenderer.h"

namespace Ember {

	void DebugRenderer::DrawLine(const Vector3f& pointA, const Vector3f& pointB, const Vector4f& color /*= Vector4f(1.0f)*/)
	{
		s_Vertices.push_back({ pointA, color });
		s_Vertices.push_back({ pointB, color });
	}

	void DebugRenderer::DrawLine(const Vector3f& pointA, const Vector3f& direction, float length, const Vector4f& color /*= Vector4f(1.0f)*/)
	{
		Vector3f endPoint = pointA + (direction * length);
		DrawLine(pointA, endPoint, color);
	}

	void DebugRenderer::DrawTriangle(const Vector3f& pointA, const Vector3f& pointB, const Vector3f& pointC, const Vector4f& color)
	{
		s_Vertices.push_back({ pointA, color });
		s_Vertices.push_back({ pointB, color });

		s_Vertices.push_back({ pointB, color });
		s_Vertices.push_back({ pointC, color });

		s_Vertices.push_back({ pointC, color });
		s_Vertices.push_back({ pointA, color });
	}

	void DebugRenderer::DrawCube(const Vector3f& center, const Vector3f& scale, const Vector4f& color)
	{
		Vector3f halfScale = scale * 0.5f;

		// Define the 8 corners of the cube
		Vector3f vertices[8] = {
			center + Vector3f(-halfScale.x, -halfScale.y, -halfScale.z), // 0: left-bottom-back
			center + Vector3f(halfScale.x, -halfScale.y, -halfScale.z), // 1: right-bottom-back
			center + Vector3f(halfScale.x,  halfScale.y, -halfScale.z), // 2: right-top-back
			center + Vector3f(-halfScale.x,  halfScale.y, -halfScale.z), // 3: left-top-back
			center + Vector3f(-halfScale.x, -halfScale.y,  halfScale.z), // 4: left-bottom-front
			center + Vector3f(halfScale.x, -halfScale.y,  halfScale.z), // 5: right-bottom-front
			center + Vector3f(halfScale.x,  halfScale.y,  halfScale.z), // 6: right-top-front
			center + Vector3f(-halfScale.x,  halfScale.y,  halfScale.z)  // 7: left-top-front
		};

		// Draw exactly 12 lines
		for (int i = 0; i < 4; ++i)
		{
			int next = (i + 1) % 4;

			// 4 Back face edges
			DrawLine(vertices[i], vertices[next], color);

			// 4 Front face edges
			DrawLine(vertices[i + 4], vertices[next + 4], color);

			// 4 Connecting depth edges
			DrawLine(vertices[i], vertices[i + 4], color);
		}
	}

	void DebugRenderer::DrawOctahedron(const Vector3f& center, float size, const Vector4f& color)
	{
		// 1. Define the 6 vertices based on the center point and size (radius)
		Vector3f top = center + Vector3f(0.0f, size, 0.0f);
		Vector3f bottom = center + Vector3f(0.0f, -size, 0.0f);
		Vector3f left = center + Vector3f(-size, 0.0f, 0.0f);
		Vector3f right = center + Vector3f(size, 0.0f, 0.0f);
		Vector3f front = center + Vector3f(0.0f, 0.0f, size);
		Vector3f back = center + Vector3f(0.0f, 0.0f, -size);

		// 2. Draw the Top Pyramid (4 lines from the top vertex to the equator)
		DrawLine(top, left, color);
		DrawLine(top, right, color);
		DrawLine(top, front, color);
		DrawLine(top, back, color);

		// 3. Draw the Bottom Pyramid (4 lines from the bottom vertex to the equator)
		DrawLine(bottom, left, color);
		DrawLine(bottom, right, color);
		DrawLine(bottom, front, color);
		DrawLine(bottom, back, color);

		// 4. Draw the Equator Square (4 lines connecting the middle vertices)
		DrawLine(left, front, color);
		DrawLine(front, right, color);
		DrawLine(right, back, color);
		DrawLine(back, left, color);
	}

	const std::vector<DebugVertex>& DebugRenderer::GetVertices()
	{
		return s_Vertices;
	}

	void DebugRenderer::Clear()
	{
		s_Vertices.clear();
	}

}