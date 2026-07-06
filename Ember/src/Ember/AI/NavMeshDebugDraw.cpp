#include "ebpch.h"
#include "NavMeshDebugDraw.h"

#include <algorithm>

namespace Ember {

	void NavMeshDebugDraw::depthMask(bool state)
	{
		m_DepthMask = state;
	}

	void NavMeshDebugDraw::texture(bool state)
	{
		m_TextureEnabled = state;
	}

	void NavMeshDebugDraw::begin(duDebugDrawPrimitives prim, float size)
	{
		m_Primitive = prim;
		m_PrimitiveSize = size;
		m_Vertices.clear();
	}

	void NavMeshDebugDraw::vertex(const float* pos, unsigned int color)
	{
		PushVertex(Vector3f(pos[0], pos[1], pos[2]), color);
	}

	void NavMeshDebugDraw::vertex(const float x, const float y, const float z, unsigned int color)
	{
		PushVertex(Vector3f(x, y, z), color);
	}

	void NavMeshDebugDraw::vertex(const float* pos, unsigned int color, const float* uv)
	{
		(void)uv;
		PushVertex(Vector3f(pos[0], pos[1], pos[2]), color);
	}

	void NavMeshDebugDraw::vertex(const float x, const float y, const float z, unsigned int color, const float u, const float v)
	{
		(void)u;
		(void)v;
		PushVertex(Vector3f(x, y, z), color);
	}

	void NavMeshDebugDraw::end()
	{
		if (m_Vertices.empty())
		{
			return;
		}

		switch (m_Primitive)
		{
			case DU_DRAW_POINTS:
			{
				const float crossHalfSize = std::max(0.01f, m_PrimitiveSize * 0.05f);
				for (const BufferedVertex& v : m_Vertices)
				{
					DebugRenderer::DrawLine(v.Position - Vector3f(crossHalfSize, 0.0f, 0.0f), v.Position + Vector3f(crossHalfSize, 0.0f, 0.0f), v.Color);
					DebugRenderer::DrawLine(v.Position - Vector3f(0.0f, crossHalfSize, 0.0f), v.Position + Vector3f(0.0f, crossHalfSize, 0.0f), v.Color);
					DebugRenderer::DrawLine(v.Position - Vector3f(0.0f, 0.0f, crossHalfSize), v.Position + Vector3f(0.0f, 0.0f, crossHalfSize), v.Color);
				}
				break;
			}
			case DU_DRAW_LINES:
			{
				for (size_t i = 0; i + 1 < m_Vertices.size(); i += 2)
				{
					DebugRenderer::DrawLine(m_Vertices[i].Position, m_Vertices[i + 1].Position, m_Vertices[i].Color);
				}
				break;
			}
			case DU_DRAW_TRIS:
			{
				for (size_t i = 0; i + 2 < m_Vertices.size(); i += 3)
				{
					DebugRenderer::DrawFilledTriangle(m_Vertices[i].Position, m_Vertices[i + 1].Position, m_Vertices[i + 2].Position, m_Vertices[i].Color);
				}
				break;
			}
			case DU_DRAW_QUADS:
			{
				for (size_t i = 0; i + 3 < m_Vertices.size(); i += 4)
				{
					DebugRenderer::DrawFilledTriangle(m_Vertices[i].Position, m_Vertices[i + 1].Position, m_Vertices[i + 2].Position, m_Vertices[i].Color);
					DebugRenderer::DrawFilledTriangle(m_Vertices[i].Position, m_Vertices[i + 2].Position, m_Vertices[i + 3].Position, m_Vertices[i].Color);
				}
				break;
			}
		}

		m_Vertices.clear();
	}

	Vector4f NavMeshDebugDraw::DecodeColor(unsigned int color)
	{
		const float r = static_cast<float>(color & 0xFF) / 255.0f;
		const float g = static_cast<float>((color >> 8) & 0xFF) / 255.0f;
		const float b = static_cast<float>((color >> 16) & 0xFF) / 255.0f;
		const float a = static_cast<float>((color >> 24) & 0xFF) / 255.0f;
		return Vector4f(r, g, b, a);
	}

	void NavMeshDebugDraw::PushVertex(const Vector3f& position, unsigned int color)
	{
		BufferedVertex& vertex = m_Vertices.emplace_back();
		vertex.Position = position;
		vertex.Color = DecodeColor(color);
	}

}
