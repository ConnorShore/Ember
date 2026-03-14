#pragma once

#include <Ember.h>

// ---------------------------------------------------------------------------
// Camera controller:  WASD = translate,  Arrow keys = rotate
// ---------------------------------------------------------------------------
class Camera3DController : public Ember::Behavior
{
public:
	void OnUpdate(Ember::TimeStep delta) override
	{
		float speed = 5.0f * delta;
		float rotSpeed = 1.5f * delta;

		// Movement (WASD)
		if (Ember::Input::IsKeyPressed(Ember::KeyCode::W))
			Transform().Position += Transform().GetForward() * speed;
		if (Ember::Input::IsKeyPressed(Ember::KeyCode::S))
			Transform().Position -= Transform().GetForward() * speed;
		if (Ember::Input::IsKeyPressed(Ember::KeyCode::D))
			Transform().Position += Transform().GetRight() * speed;
		if (Ember::Input::IsKeyPressed(Ember::KeyCode::A))
			Transform().Position -= Transform().GetRight() * speed;

		// Rotation (Arrow keys)
		if (Ember::Input::IsKeyPressed(Ember::KeyCode::Right))
			Transform().Rotation.y -= rotSpeed;
		if (Ember::Input::IsKeyPressed(Ember::KeyCode::Left))
			Transform().Rotation.y += rotSpeed;
		if (Ember::Input::IsKeyPressed(Ember::KeyCode::Up))
			Transform().Rotation.x += rotSpeed;
		if (Ember::Input::IsKeyPressed(Ember::KeyCode::Down))
			Transform().Rotation.x -= rotSpeed;
	}
};