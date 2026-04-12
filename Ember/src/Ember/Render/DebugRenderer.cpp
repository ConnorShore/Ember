#include "ebpch.h"
#include "DebugRenderer.h"

namespace Ember {

	void DebugRenderer::DrawLine(const Vector3f& pointA, const Vector3f& pointB, const Vector4f& color)
	{
		s_Vertices.push_back({ pointA, color });
		s_Vertices.push_back({ pointB, color });
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

	const std::vector<DebugVertex>& DebugRenderer::GetVertices()
	{
		return s_Vertices;
	}

	void DebugRenderer::Clear()
	{
		s_Vertices.clear();
	}

}