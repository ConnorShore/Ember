#include "ebpch.h"
#include "ScriptBindDebugDraw.h"

#include "Ember/Render/DebugRenderer.h"

namespace Ember {

	void BindDebugDraw(sol::state& state)
	{
		state.new_usertype<DebugRenderer>("Debug",
			"DrawLine", sol::overload(
				[](const Vector3f& pointA, const Vector3f& pointB, const Vector4f color) { DebugRenderer::DrawLine(pointA, pointB, color); },
				[](const Vector3f& pointA, const Vector3f& pointB) { DebugRenderer::DrawLine(pointA, pointB); },
				[](const Vector3f& pointA, const Vector3f& direction, float length, const Vector4f color) { DebugRenderer::DrawLine(pointA, direction, length, color); },
				[](const Vector3f& pointA, const Vector3f& direction, float length) { DebugRenderer::DrawLine(pointA, direction, length); }
			),
			"DrawTriangle", &DebugRenderer::DrawTriangle,
			"DrawCube", &DebugRenderer::DrawCube,
			"DrawOctahedron", &DebugRenderer::DrawOctahedron
		);
	}

}