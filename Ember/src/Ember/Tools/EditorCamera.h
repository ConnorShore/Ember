#pragma once
#include "Ember/Core/Core.h"
#include "Ember/Event/Event.h"
#include "Ember/Event/MouseEvent.h"
#include "Ember/Math/Math.h"
#include "Ember/Render/Camera.h"

namespace Ember {

	struct AABB;

	enum class EditorViewDirection
	{
		FreeFly,
		Top, Bottom, Left, Right, Front, Back
	};

	class EditorCamera : public Camera
	{
	public:
		EditorCamera() = default;
		EditorCamera(float fov, float aspectRatio, float nearClip, float farClip);

		void OnUpdate(TimeStep delta);
		void OnEvent(Event& e);

		void SnapToAxis(EditorViewDirection direction);

		// Switches projection, keeping anything on the focal plane at the same on-screen size.
		void SetProjectionMode(Camera::ProjectionType type);
		void ToggleProjectionMode();

		inline float GetDistance() const { return m_Distance; }
		inline void SetDistance(float distance) { m_Distance = distance; UpdateView(); }

		inline void SetFocalPoint(const Vector3f& focalPoint) { m_FocalPoint = focalPoint; UpdateView(); }
		inline const Vector3f& GetFocalPoint() const { return m_FocalPoint; }

		// Orbits to look at the given bounds, backing off far enough that they fill the view.
		void FocusOn(const AABB& bounds, float fillFraction = 0.75f);
		void FocusOn(const Vector3f& point, float radius);

		const Matrix4f& GetViewMatrix() const { return m_ViewMatrix; }
		Matrix4f GetViewProjection() const { return GetProjectionMatrix() * m_ViewMatrix; }

		Vector3f GetUpDirection() const;
		Vector3f GetRightDirection() const;
		Vector3f GetForwardDirection() const;
		const Vector3f& GetPosition() const { return m_Position; }
		Quaternion GetOrientation() const;

		inline void SetPitch(float pitch) { m_Pitch = pitch; UpdateView(); }
		inline void SetYaw(float yaw) { m_Yaw = yaw; UpdateView(); }

		inline void SetMoveSpeedFactor(float factor) { m_MoveSpeedFactor = factor; }
		inline void SetRotationSpeedFactor(float factor) { m_RotationSpeedFactor = factor; }
		inline void SetZoomSpeedFactor(float factor) { m_ZoomSpeedFactor = factor; }
		inline void SetPanSpeedFactor(float factor) { m_PanSpeedFactor = factor; }

		inline float GetPitch() const { return m_Pitch; }
		inline float GetYaw() const { return m_Yaw; }

		inline float GetMoveSpeedFactor() const { return m_MoveSpeedFactor; }
		inline float GetRotationSpeedFactor() const { return m_RotationSpeedFactor; }
		inline float GetZoomSpeedFactor() const { return m_ZoomSpeedFactor; }
		inline float GetPanSpeedFactor() const { return m_PanSpeedFactor; }

		inline void SetScrollDisabled(bool disabled) { m_ScrollDisabled = disabled; }
		inline bool IsScrollDisabled() const { return m_ScrollDisabled; }

	private:
		void UpdateView();

		float OrthographicSizeForDistance() const;
		void SyncOrthographicSize();

		bool OnMouseScroll(MouseScrolledEvent& e);

		void MousePan(const Vector2f& delta);
		void MouseOrbit(const Vector2f& delta);
		void MouseRotate(const Vector2f& delta);
		void MouseZoom(float delta);

		Vector3f CalculatePosition() const;

		std::pair<float, float> PanSpeed() const;
		float ZoomSpeed() const;

	private:
		Matrix4f m_ViewMatrix = Matrix4f(1.0f);

		Vector3f m_Position = { 0.0f, 0.0f, 0.0f };
		Vector3f m_FocalPoint = { 0.0f, 0.0f, 0.0f };

		Vector2f m_InitialMousePosition = { 0.0f, 0.0f };

		float m_Distance = 10.0f;
		float m_Pitch = 0.0f, m_Yaw = 0.0f;

		float m_MoveSpeedFactor = 1.0f;
		float m_RotationSpeedFactor = 1.0f;
		float m_ZoomSpeedFactor = 1.0f;
		float m_PanSpeedFactor = 1.0f;

		bool m_ScrollDisabled = false;
	};

}