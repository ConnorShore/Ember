#include "ebpch.h"
#include "Camera.h"

namespace Ember {

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

	void Camera::SetViewportSize(unsigned int width, unsigned int height)
	{
		m_AspectRatio = (float)width / (float)height;
		CalculateProjectionMatrix();
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