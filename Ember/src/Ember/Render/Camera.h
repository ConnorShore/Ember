#pragma once

#include "Ember/Math/Math.h"

namespace Ember {

	//////////////////////////////////////////////////////////////////////////
	// Camera
	//////////////////////////////////////////////////////////////////////////

	class Camera
	{
	public:
		Camera() = default;
		virtual ~Camera() = default;

		virtual const Matrix4f& GetProjectionMatrix() const = 0;
	};

	//////////////////////////////////////////////////////////////////////////
	// Orthographic Camera
	//////////////////////////////////////////////////////////////////////////

	class OrthographicCamera : public Camera
	{
	public:
		OrthographicCamera(float left, float right, float bottom, float top, float zNear = -1.0f, float zFar = 1.0f);
		virtual ~OrthographicCamera() = default;

		virtual const Matrix4f& GetProjectionMatrix() const override { return m_ProjectionMatrix; }
		const Matrix4f& GetViewMatrix() const { return m_ViewMatrix; }
		const Matrix4f& GetViewProjectionMatrix() const { return m_ViewProjectionMatrix; }
		const Vector3f& GetPosition() const { return m_Position; }
		float GetRotation() const { return m_Rotation; }

		void SetPosition(const Vector3f& position) { m_Position = position; CalculateViewMatrix(); }
		void SetRotation(float rotation) { m_Rotation = rotation; CalculateViewMatrix(); }

	private:
		void CalculateViewMatrix();

	private:
		Matrix4f m_ProjectionMatrix, m_ViewMatrix, m_ViewProjectionMatrix;
		Vector3f m_Position;
		float m_Rotation;
	};

}