#pragma once

#include "Ember/Render/DebugRenderer.h"

#include <DebugDraw.h>

#include <vector>

namespace Ember {

	class NavMeshDebugDraw : public duDebugDraw
	{
	public:
		void depthMask(bool state) override;
		void texture(bool state) override;

		void begin(duDebugDrawPrimitives prim, float size = 1.0f) override;
		void vertex(const float* pos, unsigned int color) override;
		void vertex(const float x, const float y, const float z, unsigned int color) override;
		void vertex(const float* pos, unsigned int color, const float* uv) override;
		void vertex(const float x, const float y, const float z, unsigned int color, const float u, const float v) override;
		void end() override;

	private:
		struct BufferedVertex
		{
			Vector3f Position = Vector3f(0.0f);
			Vector4f Color = Vector4f(1.0f);
		};

		static Vector4f DecodeColor(unsigned int color);
		void PushVertex(const Vector3f& position, unsigned int color);

	private:
		duDebugDrawPrimitives m_Primitive = DU_DRAW_LINES;
		float m_PrimitiveSize = 1.0f;
		bool m_DepthMask = true;
		bool m_TextureEnabled = false;
		std::vector<BufferedVertex> m_Vertices;
	};

}
