#pragma once

#include "Ember/Math/Math.h"

namespace Ember {

	class Camera
	{
	public:
		enum class ProjectionType { Orthographic = 0, Perspective = 1 };

	public:
		Camera();
		virtual ~Camera() = default;

		void SetPerspective(float fov, float nearClip, float farClip);
		void SetOrthographic(float size, float nearClip, float farClip);
		void SetViewportSize(unsigned int width, unsigned int height);

		inline void SetProjectionType(ProjectionType type) { m_ProjectionType = type; }
		inline const Matrix4f GetProjectionMatrix() const { return m_ProjectionMatrix; }

	private:
		void CalculateProjectionMatrix();

	private:
		ProjectionType m_ProjectionType = ProjectionType::Perspective;
		Matrix4f m_ProjectionMatrix = Matrix4f(1.0f);
		float m_AspectRatio;	// (height / width)

		struct OrthographicProps
		{
			float Size;
			float NearClip;
			float FarClip;
		} m_OrthographicProps;

		struct PerspectiveProps
		{
			float FieldOfView;
			float NearClip;
			float FarClip;
		} m_PerspectiveProps;

	};

}