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

	void ParticleManager::Emit(const ParticleEmitterComponent& component, const Vector3f& position)
	{
		Particle& particle = m_ParticlePool[m_ParticleIndex];

		// Active the particle
		particle.Active = true;
		particle.Position = position;

		particle.GravityMultiplier = component.GravityMultiplier;

		particle.Drag = component.Drag;
		particle.AngularVelocity = component.AngularVelocity + component.AngularVelocityVariation * (Random::Float() - 0.5f);

		// Apply props with variation when necessary
		particle.Velocity = component.Velocity + component.VelocityVariation * Vector3f(
			Random::Float() - 0.5f,
			Random::Float() - 0.5f,
			Random::Float() - 0.5f
		);

		particle.ColorBegin = component.ColorBegin;
		particle.ColorEnd = component.ColorEnd;

		particle.ScaleBegin = component.ScaleBegin + component.ScaleVariation * (Random::Float() - 0.5f);
		particle.ScaleEnd = component.ScaleEnd;

		particle.Lifetime = component.Lifetime + component.LifetimeVariation * (Random::Float() - 0.5f);
		particle.LifeRemaining = particle.Lifetime;

		particle.AlignWithVelocity = component.AlignWithVelocity;
		particle.StretchFactor = component.StretchFactor;

		particle.TextureHandle = component.TextureHandle;

		// Move the index backwards. If it hits 0, wrap back around to the top
		m_ParticleIndex = (m_ParticleIndex == 0) ? m_MaxParticles - 1 : m_ParticleIndex - 1;
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

			particle.Velocity *= std::exp(-particle.Drag * (float)delta);	// Apply drag
			particle.Velocity += gravity * particle.GravityMultiplier * (float)delta;	// Apply gravity
			particle.Position += particle.Velocity * (float)delta;

			particle.Rotation += particle.AngularVelocity * (float)delta;

			// Interpolate color and scale based on Life ratio!
			float lifeRatio = particle.LifeRemaining / particle.Lifetime;

			// Update particle color/scale based on emitter's start/end props and life ratio
			particle.CurrentColor = Math::Lerp(particle.ColorEnd, particle.ColorBegin, lifeRatio);
			particle.CurrentScale = Math::Lerp(particle.ScaleEnd, particle.ScaleBegin, lifeRatio);
		}
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