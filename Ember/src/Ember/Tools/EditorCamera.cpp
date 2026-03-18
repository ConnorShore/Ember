#include "ebpch.h"
#include "EditorCamera.h"
#include "Ember/Input/Input.h"
#include "Ember/Input/InputCode.h"
#include "Ember/Input/KeyCode.h"
#include "Ember/Input/MouseCode.h"
#include "Ember/Event/KeyEvent.h"

namespace Ember {

	EditorCamera::EditorCamera(float fov, float aspectRatio, float nearClip, float farClip)
	{
		// FIX: Let the base class set up all the projection properties!
		SetProjectionType(Camera::ProjectionType::Perspective);
		SetPerspective(fov, nearClip, farClip);
		UpdateView();
	}

	void EditorCamera::UpdateView()
	{
		m_Position = CalculatePosition();

		Quaternion orientation = GetOrientation();
		m_ViewMatrix = Math::Translate(m_Position) * Math::ToMatrix4f(orientation);
		m_ViewMatrix = Math::Inverse(m_ViewMatrix);
	}

	std::pair<float, float> EditorCamera::PanSpeed() const
	{
		// FIX: Ask the base class for the viewport size instead of tracking it ourselves
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
		if (Input::IsMouseButtonPressed(MouseButton::Middle))
		{
			if (Input::IsKeyPressed(KeyCode::LeftShift))
				MousePan(mouseDelta);
			else
				MouseOrbit(mouseDelta);
		}
		else if (Input::IsMouseButtonPressed(MouseButton::Right))
		{
			// Fly Camera: Rotate while holding RMB
			MouseRotate(mouseDelta);

			// Fly Camera: Move Focal Point with WASD
			float moveSpeed = m_Distance * m_MoveSpeedFactor * delta.Seconds();
			if (Input::IsKeyPressed(KeyCode::W)) m_FocalPoint += GetForwardDirection() * moveSpeed;
			if (Input::IsKeyPressed(KeyCode::S)) m_FocalPoint -= GetForwardDirection() * moveSpeed;
			if (Input::IsKeyPressed(KeyCode::A)) m_FocalPoint -= GetRightDirection() * moveSpeed;
			if (Input::IsKeyPressed(KeyCode::D)) m_FocalPoint += GetRightDirection() * moveSpeed;
		}

		UpdateView();
	}

	void EditorCamera::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		EB_DISPATCH_EVENT(MouseScrolledEvent, OnMouseScroll);
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
		float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;
		m_Yaw += yawSign * delta.x * m_RotationSpeedFactor;
		m_Pitch += delta.y * m_RotationSpeedFactor;
	}

	void EditorCamera::MouseRotate(const Vector2f& delta)
	{
		Vector3f currentPosition = CalculatePosition();

		float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;
		m_Yaw += yawSign * delta.x * m_RotationSpeedFactor;
		m_Pitch += delta.y * m_RotationSpeedFactor;

		// Gimbel lock
		m_Pitch = std::max(m_Pitch, -1.56f);
		m_Pitch = std::min(m_Pitch, 1.56f);

		m_FocalPoint = currentPosition + GetForwardDirection() * m_Distance;
	}

	void EditorCamera::MouseZoom(float delta)
	{
		m_Distance -= delta * ZoomSpeed();
		if (m_Distance < 1.0f)
		{
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