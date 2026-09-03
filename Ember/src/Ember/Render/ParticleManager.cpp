#include "ebpch.h"
#include "ParticleManager.h"

#include "Ember/Core/Application.h"
#include "Ember/Core/Random.h"
#include "Ember/ECS/System/PhysicsSystem.h"

namespace Ember {


	ParticleManager::ParticleManager(uint32_t maxParticles /*= 10000*/)
		: m_MaxParticles(maxParticles)
	{
		m_ParticlePool.resize(maxParticles);
		m_ParticleIndex = maxParticles - 1; // Start at the end of the pool so the first particle will be at index 0
	}

	void ParticleManager::Emit(const ParticleEffectProperties& properties, const Vector3f& position, const Quaternion& worldRotation)
	{
		Particle& particle = m_ParticlePool[m_ParticleIndex];

		// Active the particle
		particle.Active = true;
		particle.Position = position;

		particle.GravityMultiplier = properties.GravityMultiplier;

		particle.Drag = properties.Drag;
		particle.AngularVelocity = properties.AngularVelocity + properties.AngularVelocityVariation * (Random::Float() - 0.5f);

		// Must be seeded, not inherited: the slot still holds the accumulated rotation of its last occupant
		particle.Rotation = Random::Float() * Math::TwoPi;

		// Apply props with variation when necessary, then rotate into world space
		Vector3f localVelocity = properties.Velocity + properties.VelocityVariation * Vector3f(
			Random::Float() - 0.5f,
			Random::Float() - 0.5f,
			Random::Float() - 0.5f
		);
		particle.Velocity = worldRotation * localVelocity;

		particle.ColorBegin = properties.ColorBegin;
		particle.ColorEnd = properties.ColorEnd;
		particle.ColorIntensity = properties.ColorIntensity;

		particle.ScaleBegin = properties.ScaleBegin + properties.ScaleVariation * (Random::Float() - 0.5f);
		particle.ScaleEnd = properties.ScaleEnd;

		particle.Lifetime = properties.Lifetime + properties.LifetimeVariation * (Random::Float() - 0.5f);
		particle.LifeRemaining = particle.Lifetime;

		particle.AlignWithVelocity = properties.AlignWithVelocity;
		particle.StretchFactor = properties.StretchFactor;

		particle.TextureHandle = properties.TextureHandle;
		particle.BlendMode = properties.BlendMode;

		// Move the index backwards. If it hits 0, wrap back around to the top
		m_ParticleIndex = (m_ParticleIndex == 0) ? m_MaxParticles - 1 : m_ParticleIndex - 1;
	}

	void ParticleManager::EmitBurst(const ParticleEffectProperties& properties, const Vector3f& position, uint32_t count, const Quaternion& worldRotation)
	{
		for (uint32_t i = 0; i < count; i++)
			Emit(properties, position, worldRotation);
	}

	void ParticleManager::OnUpdate(TimeStep delta)
	{
		auto physicsSystem = Application::Instance().GetSystem<PhysicsSystem>();
		Vector3f gravity = physicsSystem->GetSettings().GravityVector * physicsSystem->GetSettings().GravityStrength;

		for (auto& particle : m_ParticlePool)
		{
			if (!particle.Active)
				continue;

			if (particle.LifeRemaining <= 0.0f)
			{
				particle.Active = false;
				continue;
			}

			particle.LifeRemaining -= delta;

			particle.Velocity *= Math::Exp(-particle.Drag * (float)delta);	// Apply drag
			particle.Velocity += gravity * particle.GravityMultiplier * (float)delta;	// Apply gravity
			particle.Position += particle.Velocity * (float)delta;

			particle.Rotation += particle.AngularVelocity * (float)delta;

			// Interpolate color and scale based on Life ratio!
			float lifeRatio = particle.LifeRemaining / particle.Lifetime;

			// Update particle color/scale based on emitter's start/end props and life ratio
			particle.CurrentColor = Math::Lerp(particle.ColorEnd, particle.ColorBegin, lifeRatio);
			particle.CurrentColor = Vector4f(Vector3f(particle.CurrentColor) * particle.ColorIntensity, particle.CurrentColor.a);
			particle.CurrentScale = Math::Lerp(particle.ScaleEnd, particle.ScaleBegin, lifeRatio);
		}
	}

	uint32_t ParticleManager::GetActiveParticleCount() const
	{
		uint32_t count = 0;
		for (const auto& particle : m_ParticlePool)
		{
			if (particle.Active)
				count++;
		}
		return count;
	}

	void ParticleManager::Reset()
	{
		for (auto& particle : m_ParticlePool)
		{
			particle.Active = false;
		}

		m_ParticleIndex = m_MaxParticles - 1; // Reset index to the end of the pool
	}

}