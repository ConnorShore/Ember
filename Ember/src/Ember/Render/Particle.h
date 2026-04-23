#pragma once

#include "Ember/Asset/UUID.h"
#include "Ember/Math/Math.h"

namespace Ember {

	struct Particle
	{
		Vector3f Position;
		Vector3f Velocity;

		float GravityMultiplier = 0.0f;

		Vector4f ColorBegin, ColorEnd;
		float ScaleBegin, ScaleEnd;

		float CurrentScale;
		Vector4f CurrentColor;

		UUID TextureHandle;
		// TODO: Support sprite sheets for animated particles

		float LifeRemaining = 0.0f;
		float Lifetime = 1.0f;
		bool Active = false;
	};

	struct ParticleVertex
	{
		Vector3f Position;
		float Scale;
		Vector4f Color;
		uint32_t TexIndex;
	};

}