#pragma once

#include "Ember/ECS/Component/Components.h"
#include "Ember/Render/Particle.h"

#include <vector>

namespace Ember {

	class ParticleManager
	{
	public:
		ParticleManager(uint32_t maxParticles = 10000);
		virtual ~ParticleManager() = default;

		void Emit(const ParticleEmitterComponent& component, const Vector3f& position);
		void OnUpdate(TimeStep delta);

		uint32_t GetMaxParticles() const { return m_MaxParticles; }

		const std::vector<Particle>& GetParticles() const { return m_ParticlePool; }

	private:
		std::vector<Particle> m_ParticlePool;
		uint32_t m_MaxParticles = 10000;
		uint32_t m_ParticleIndex = 9999; // Start at the end of the pool so the first particle will be at index 0
	};

}