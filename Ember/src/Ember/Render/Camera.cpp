#include "ebpch.h"
#include "Camera.h"

#include "Ember/Render/DebugRenderer.h"

#include <algorithm>

namespace Ember {

	static Vector3f ProjectFrustumCorner(const Matrix4f& inverseViewProjection, const Vector4f& ndcPoint)
	{
		Vector4f worldPoint = inverseViewProjection * ndcPoint;
		worldPoint /= worldPoint.w;
		return Vector3f(worldPoint.x, worldPoint.y, worldPoint.z);
	}

	static void DrawFrustumEdges(const Vector3f corners[8], const Vector4f& color)
	{
		DebugRenderer::DrawLine(corners[0], corners[1], color);
		DebugRenderer::DrawLine(corners[1], corners[2], color);
		DebugRenderer::DrawLine(corners[2], corners[3], color);
		DebugRenderer::DrawLine(corners[3], corners[0], color);

		DebugRenderer::DrawLine(corners[4], corners[5], color);
		DebugRenderer::DrawLine(corners[5], corners[6], color);
		DebugRenderer::DrawLine(corners[6], corners[7], color);
		DebugRenderer::DrawLine(corners[7], corners[4], color);

		DebugRenderer::DrawLine(corners[0], corners[4], color);
		DebugRenderer::DrawLine(corners[1], corners[5], color);
		DebugRenderer::DrawLine(corners[2], corners[6], color);
		DebugRenderer::DrawLine(corners[3], corners[7], color);
	}

	Camera::Camera()
	{
		CalculateProjectionMatrix();
	}

	void Camera::SetPerspective(float fov, float nearClip, float farClip)
	{
		m_PerspectiveProps = { fov, nearClip, farClip };
		CalculateProjectionMatrix();
	}

	void Camera::SetOrthographic(float size, float nearClip, float farClip)
	{
		m_OrthographicProps = { size, nearClip, farClip };
		CalculateProjectionMatrix();
	}

	void Camera::SetViewportSize(uint32_t width, uint32_t height)
	{
		if (width == 0 || height == 0)
			return;

		m_ViewportSize = Vector2f(width, height);
		m_AspectRatio = (float)width / (float)height;
		CalculateProjectionMatrix();
	}

	void Camera::DrawFrustum(const Matrix4f& cameraTransform, bool isSelected) const
	{
		Matrix4f projection = m_ProjectionMatrix;
		if (!isSelected)
		{
			if (m_ProjectionType == ProjectionType::Perspective)
			{
				float previewFar = std::min(m_PerspectiveProps.FarClip, m_PerspectiveProps.NearClip + 1.0f);
				if (previewFar <= m_PerspectiveProps.NearClip)
					previewFar = m_PerspectiveProps.FarClip;

				projection = Math::Perspective(m_PerspectiveProps.FieldOfView, m_AspectRatio, m_PerspectiveProps.NearClip, previewFar);
			}
			else if (m_ProjectionType == ProjectionType::Orthographic)
			{
				float previewSize = std::min(m_OrthographicProps.Size, 2.0f);
				float left = -previewSize * m_AspectRatio * 0.5f;
				float right = previewSize * m_AspectRatio * 0.5f;
				float top = previewSize * 0.5f;
				float bottom = -previewSize * 0.5f;
				projection = Math::Orthographic(left, right, bottom, top, -0.5f, 0.5f);
			}
		}

		Matrix4f inverseViewProjection = Math::Inverse(projection * Math::Inverse(cameraTransform));
		Vector3f corners[8] = {
			ProjectFrustumCorner(inverseViewProjection, Vector4f(-1.0f, -1.0f, -1.0f, 1.0f)),
			ProjectFrustumCorner(inverseViewProjection, Vector4f( 1.0f, -1.0f, -1.0f, 1.0f)),
			ProjectFrustumCorner(inverseViewProjection, Vector4f( 1.0f,  1.0f, -1.0f, 1.0f)),
			ProjectFrustumCorner(inverseViewProjection, Vector4f(-1.0f,  1.0f, -1.0f, 1.0f)),
			ProjectFrustumCorner(inverseViewProjection, Vector4f(-1.0f, -1.0f,  1.0f, 1.0f)),
			ProjectFrustumCorner(inverseViewProjection, Vector4f( 1.0f, -1.0f,  1.0f, 1.0f)),
			ProjectFrustumCorner(inverseViewProjection, Vector4f( 1.0f,  1.0f,  1.0f, 1.0f)),
			ProjectFrustumCorner(inverseViewProjection, Vector4f(-1.0f,  1.0f,  1.0f, 1.0f))
		};

		Vector4f color = isSelected
			? Vector4f(1.0f, 0.85f, 0.15f, 1.0f)
			: Vector4f(0.2f, 0.7f, 1.0f, 1.0f);

		DrawFrustumEdges(corners, color);
	}

	void Camera::CalculateProjectionMatrix()
	{
		if (m_ProjectionType == ProjectionType::Perspective)
		{
			m_ProjectionMatrix = Math::Perspective(m_PerspectiveProps.FieldOfView, m_AspectRatio, 
				m_PerspectiveProps.NearClip, m_PerspectiveProps.FarClip);
		}
		else if (m_ProjectionType == ProjectionType::Orthographic)
		{
			// Scale ortho bounds by aspect ratio so content isn't stretched
			float left = -m_OrthographicProps.Size * m_AspectRatio * 0.5f;
			float right = m_OrthographicProps.Size * m_AspectRatio * 0.5f;
			float top = m_OrthographicProps.Size * 0.5f;
			float bottom = -m_OrthographicProps.Size * 0.5f;
			m_ProjectionMatrix = Math::Orthographic(left, right, bottom, top, m_OrthographicProps.NearClip, m_OrthographicProps.FarClip);
		}
		else
		{
			EB_CORE_ASSERT(false, "Unknown projection type set!");
		}
	}

}