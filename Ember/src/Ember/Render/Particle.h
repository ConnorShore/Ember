#pragma once

#include "Ember/Asset/UUID.h"
#include "Ember/Math/Math.h"

namespace Ember {

	struct Particle
	{
		Vector3f Position;
		Vector3f Velocity;

		float GravityMultiplier = 0.0f;

		float Drag = 0.1f; // How much air resistance slows down particles over time
		float AngularVelocity = 0.0f; // How fast particles rotate
		float Rotation = 0.2f;

		bool AlignWithVelocity = false; // If true, particles will rotate to face the direction they're moving
		float StretchFactor = 0.0f; // If AlignWithVelocity is true, this controls how much particles stretch in the direction of movement (0 = no stretch, 1 = full stretch)

		Vector4f ColorBegin, ColorEnd;
		float ScaleBegin, ScaleEnd;

		float CurrentScale;
		Vector4f CurrentColor;

		UUID TextureHandle;
		// TODO: Support sprite sheets for animated particles
		// TODO: Add dampening to velocity for "thicker" particles or "thicker" air

		float LifeRemaining = 0.0f;
		float Lifetime = 1.0f;
		bool Active = false;
	};

	struct ParticleVertex
	{
		Vector3f Position;
		float Rotation;
		Vector2f Scale;
		Vector4f Color;
		uint32_t TexIndex;
	};

}