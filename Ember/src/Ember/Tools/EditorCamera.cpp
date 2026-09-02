#include "ebpch.h"
#include "EditorCamera.h"
#include "Ember/Input/Input.h"
#include "Ember/Input/InputCode.h"
#include "Ember/Input/KeyCode.h"
#include "Ember/Input/MouseCode.h"
#include "Ember/Event/KeyEvent.h"
#include "Ember/Render/Frustum.h"

namespace Ember {

	EditorCamera::EditorCamera(float fov, float aspectRatio, float nearClip, float farClip)
	{
		// FIX: Let the base class set up all the projection properties!
		SetProjectionType(Camera::ProjectionType::Perspective);
		SetPerspective(fov, nearClip, farClip);
		UpdateView();
	}

	// Recalculate the view matrix from the orbit position around the focal point
	void EditorCamera::UpdateView()
	{
		m_Position = CalculatePosition();

		Quaternion orientation = GetOrientation();
		m_ViewMatrix = Math::Translate(m_Position) * Math::ToMatrix4f(orientation);
		m_ViewMatrix = Math::Inverse(m_ViewMatrix);

		// Zoom is m_Distance in both projections, so the ortho box follows it.
		if (IsOrthographic())
			SyncOrthographicSize();
	}

	// The view height the perspective frustum spans at the focal point.
	float EditorCamera::OrthographicSizeForDistance() const
	{
		return 2.0f * m_Distance * Math::Tan(Math::Radians(GetPerspectiveProps().FieldOfView) * 0.5f);
	}

	// Derived from the orbit, so zoom, pan and focus need no separate ortho path.
	void EditorCamera::SyncOrthographicSize()
	{
		// Centre the depth slab on the focal point so orbiting cannot clip what is behind it.
		float depth = GetPerspectiveProps().FarClip;
		SetOrthographic(OrthographicSizeForDistance(), -depth, depth);
	}

	void EditorCamera::SetProjectionMode(Camera::ProjectionType type)
	{
		if (GetProjectionType() == type)
			return;

		// Perspective rebuilds from its own props; ortho has to be derived.
		SetProjectionType(type);
		if (IsOrthographic())
			SyncOrthographicSize();
	}

	void EditorCamera::ToggleProjectionMode()
	{
		SetProjectionMode(IsOrthographic() ? Camera::ProjectionType::Perspective : Camera::ProjectionType::Orthographic);
	}

	void EditorCamera::FocusOn(const AABB& bounds, float fillFraction)
	{
		Vector3f center = (bounds.WorldMin + bounds.WorldMax) * 0.5f;
		Vector3f extent = (bounds.WorldMax - bounds.WorldMin) * 0.5f;

		// Frame the bounding sphere so the result does not depend on which way the box is turned.
		float radius = Math::Length(extent);
		if (fillFraction > 0.0f)
			radius /= fillFraction;

		FocusOn(center, radius);
	}

	void EditorCamera::FocusOn(const Vector3f& point, float radius)
	{
		m_FocalPoint = point;

		// A degenerate bound (a light, an empty) still needs a usable viewing distance.
		radius = Math::Max(radius, 0.1f);

		// Fit against the narrower of the two field of views so a wide viewport never clips the sides.
		float verticalFov = Math::Radians(GetPerspectiveProps().FieldOfView);
		float aspectRatio = Math::Max(GetAspectRatio(), 0.01f);
		float horizontalFov = 2.0f * Math::Atan(Math::Tan(verticalFov * 0.5f) * aspectRatio);
		float fov = Math::Min(verticalFov, horizontalFov);

		// Only perspective has a near plane to stay clear of.
		float minimumDistance = IsOrthographic() ? 0.01f : GetPerspectiveProps().NearClip * 4.0f;

		m_Distance = Math::Max(radius / Math::Tan(fov * 0.5f), minimumDistance);
		UpdateView();
	}

	// Pan speed scales with viewport size via a quadratic curve so it feels consistent
	std::pair<float, float> EditorCamera::PanSpeed() const
	{
		float x = std::min(GetViewportSize().x / 1000.0f, 2.4f); // max = 2.4f
		float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

		float y = std::min(GetViewportSize().y / 1000.0f, 2.4f); // max = 2.4f
		float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

		return { xFactor, yFactor };
	}

	float EditorCamera::ZoomSpeed() const
	{
		float distance = m_Distance * 0.2f;
		distance = std::max(distance, 0.0f);
		float speed = distance * distance;
		speed *= m_ZoomSpeedFactor;
		speed = std::min(speed, 100.0f); // max speed = 100
		return speed;
	}

	void EditorCamera::OnUpdate(TimeStep delta)
	{
		// Get current mouse position and calculate how much it moved since last frame
		Vector2f mousePos = Input::GetMousePosition();
		Vector2f mouseDelta = (mousePos - m_InitialMousePosition) * 0.003f;
		m_InitialMousePosition = mousePos;

		// Handle Input Modifiers
		if (Input::IsMouseButtonDown(MouseButton::Middle))
		{
			if (Input::IsModifierActive(KeyModifier::Shift))
				MousePan(mouseDelta);
			else
				MouseOrbit(mouseDelta);
		}
		else if (Input::IsMouseButtonDown(MouseButton::Right))
		{
			// Fly Camera: Rotate while holding RMB
			MouseRotate(mouseDelta);

			// Fly Camera: Move Focal Point with WASD
			float moveSpeed = m_Distance * m_MoveSpeedFactor * delta.Seconds();
			if (Input::IsKeyDown(KeyCode::W))
				m_FocalPoint += GetForwardDirection() * moveSpeed;
			if (Input::IsKeyDown(KeyCode::S))
				m_FocalPoint -= GetForwardDirection() * moveSpeed;
			if (Input::IsKeyDown(KeyCode::A))
				m_FocalPoint -= GetRightDirection() * moveSpeed;
			if (Input::IsKeyDown(KeyCode::D))
				m_FocalPoint += GetRightDirection() * moveSpeed;
		}

		UpdateView();
	}

	void EditorCamera::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		EB_DISPATCH_EVENT(MouseScrolledEvent, OnMouseScroll);
	}

	// Yaw is measured from -Z and positive pitch looks downward, matching the euler order GetOrientation() uses
	void EditorCamera::SnapToAxis(EditorViewDirection direction)
	{
		switch (direction)
		{
		case EditorViewDirection::Top:
			m_Pitch = Math::Radians(89.9f);  // Look straight down (almost, to stay clear of gimbal lock)
			m_Yaw = 0.0f;                    // Keep screen-up pointed towards -Z
			break;
		case EditorViewDirection::Bottom:
			m_Pitch = Math::Radians(-89.9f); // Look straight up
			m_Yaw = 0.0f;                    // Screen-up lands on +Z, mirroring Top
			break;
		case EditorViewDirection::Right:  // Looking down -X
			m_Pitch = 0.0f;
			m_Yaw = Math::Radians(-90.0f);
			break;
		case EditorViewDirection::Left:   // Looking down +X
			m_Pitch = 0.0f;
			m_Yaw = Math::Radians(90.0f);
			break;
		case EditorViewDirection::Front:  // Looking down +Z, so an entity facing -Z shows its front
			m_Pitch = 0.0f;
			m_Yaw = Math::Radians(180.0f);
			break;
		case EditorViewDirection::Back:   // Looking down -Z
			m_Pitch = 0.0f;
			m_Yaw = 0.0f;
			break;
		case EditorViewDirection::FreeFly:
			// Do nothing to rotation, just return to normal flight
			break;
		}

		UpdateView();
	}

	bool EditorCamera::OnMouseScroll(MouseScrolledEvent& e)
	{
		float delta = e.GetYOffset() * 0.1f;
		MouseZoom(delta);
		UpdateView();
		return false;
	}

	void EditorCamera::MousePan(const Vector2f& delta)
	{
		auto [xSpeed, ySpeed] = PanSpeed();
		m_FocalPoint += -GetRightDirection() * delta.x * xSpeed * m_Distance;
		m_FocalPoint += GetUpDirection() * delta.y * ySpeed * m_Distance;
	}

	void EditorCamera::MouseOrbit(const Vector2f& delta)
	{
		// Flip yaw direction when upside-down to keep controls intuitive
		float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;
		m_Yaw += yawSign * delta.x * m_RotationSpeedFactor;
		m_Pitch += delta.y * m_RotationSpeedFactor;
	}

	void EditorCamera::MouseRotate(const Vector2f& delta)
	{
		// In fly mode, rotate around the camera's current position instead of the focal point
		Vector3f currentPosition = CalculatePosition();

		float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;
		m_Yaw += yawSign * delta.x * m_RotationSpeedFactor;
		m_Pitch += delta.y * m_RotationSpeedFactor;

		// Clamp pitch near +/-90 degrees to avoid gimbal lock
		m_Pitch = std::max(m_Pitch, -1.56f);
		m_Pitch = std::min(m_Pitch, 1.56f);

		// Move focal point so the orbit radius stays consistent after rotation
		m_FocalPoint = currentPosition + GetForwardDirection() * m_Distance;
	}

	void EditorCamera::MouseZoom(float delta)
	{
		if (m_ScrollDisabled)
			return;

		m_Distance -= delta * ZoomSpeed();

		// Ortho just shrinks the box, so it never needs the focal point nudged along.
		if (IsOrthographic())
		{
			m_Distance = Math::Max(m_Distance, 0.01f);
			return;
		}

		if (m_Distance < 1.0f)
		{
			// Push focal point forward instead of allowing negative distance
			m_FocalPoint += GetForwardDirection();
			m_Distance = 1.0f;
		}
	}

	Vector3f EditorCamera::GetUpDirection() const
	{
		return Math::Rotate(GetOrientation(), Vector3f(0.0f, 1.0f, 0.0f));
	}

	Vector3f EditorCamera::GetRightDirection() const
	{
		return Math::Rotate(GetOrientation(), Vector3f(1.0f, 0.0f, 0.0f));
	}

	Vector3f EditorCamera::GetForwardDirection() const
	{
		return Math::Rotate(GetOrientation(), Vector3f(0.0f, 0.0f, -1.0f));
	}

	Vector3f EditorCamera::CalculatePosition() const
	{
		return m_FocalPoint - GetForwardDirection() * m_Distance;
	}

	Quaternion EditorCamera::GetOrientation() const
	{
		return Quaternion(Vector3f(-m_Pitch, -m_Yaw, 0.0f));
	}

}