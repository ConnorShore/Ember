#pragma once

#include "Ember/Math/Math.h"

namespace Ember {

	class Camera
	{
	public:
		enum class ProjectionType { Orthographic = 0, Perspective = 1, Count };
		static std::string GetProjectionTypeName(ProjectionType type) { return type == ProjectionType::Orthographic ? "Orthographic" : "Perspective"; }

		struct OrthographicProps
		{
			float Size = 5.0f;
			float NearClip = -1.0f;
			float FarClip = 1.0f;
		};

		struct PerspectiveProps
		{
			float FieldOfView = 70.0f;
			float NearClip = 1.0f;
			float FarClip = 100.0f;
		};

	public:
		Camera();
		virtual ~Camera() = default;

		void SetPerspective(float fov, float nearClip, float farClip);
		void SetOrthographic(float size, float nearClip, float farClip);
		void SetViewportSize(unsigned int width, unsigned int height);

		inline void SetProjectionType(ProjectionType type) { m_ProjectionType = type; }

		inline const Matrix4f GetProjectionMatrix() const { return m_ProjectionMatrix; }
		inline void SetProjectionMatrix(const Matrix4f& matrix) { m_ProjectionMatrix = matrix; }

		inline const Vector2f& GetViewportSize() const { return m_ViewportSize; }

		inline ProjectionType GetProjectionType() const { return m_ProjectionType; }
		inline OrthographicProps& GetOrthographicProps() { return m_OrthographicProps; }
		inline PerspectiveProps& GetPerspectiveProps() { return m_PerspectiveProps; }

	private:
		void CalculateProjectionMatrix();

	private:
		ProjectionType m_ProjectionType = ProjectionType::Perspective;
		Matrix4f m_ProjectionMatrix = Matrix4f(1.0f);
		Vector2f m_ViewportSize;
		float m_AspectRatio;	// (height / width)

		OrthographicProps m_OrthographicProps;
		PerspectiveProps m_PerspectiveProps;

	};

}