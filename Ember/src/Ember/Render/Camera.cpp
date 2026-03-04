#include "ebpch.h"
#include "Camera.h"

namespace Ember {

	OrthographicCamera::OrthographicCamera(float left, float right, float bottom, float top, float zNear /*= -1.0f*/, float zFar /*= 1.0f*/) 
		: m_ProjectionMatrix(Math::Orthographic(left, right, bottom, top, zNear, zFar)),
		m_ViewMatrix(Matrix4f(1.0f)),
		m_Position(Vector3f(0.0f)),
		m_Rotation(0.0f)
	{
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

	void OrthographicCamera::CalculateViewMatrix()
	{
		Matrix4f transform = Math::Translate(m_Position) * Math::Rotate(m_Rotation, Vector3f(0, 0, 1));
		m_ViewMatrix = Math::Inverse(transform);
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

}