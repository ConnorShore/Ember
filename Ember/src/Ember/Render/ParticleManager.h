#pragma once

#include "Ember/Asset/ParticleEffect.h"
#include "Ember/Render/Particle.h"
#include "Ember/Core/Time.h"

#include <vector>

namespace Ember {

	class ParticleManager
	{
	public:
		ParticleManager(uint32_t maxParticles = 10000);
		virtual ~ParticleManager() = default;

		void Emit(const ParticleEffectProperties& properties, const Vector3f& position, const Quaternion& worldRotation = Quaternion(1.0f, 0.0f, 0.0f, 0.0f));
		void EmitBurst(const ParticleEffectProperties& properties, const Vector3f& position, uint32_t count, const Quaternion& worldRotation = Quaternion(1.0f, 0.0f, 0.0f, 0.0f));
		void OnUpdate(TimeStep delta);

		void Reset();

		uint32_t GetMaxParticles() const { return m_MaxParticles; }
		uint32_t GetActiveParticleCount() const;

		const std::vector<Particle>& GetParticles() const { return m_ParticlePool; }

	private:
		std::vector<Particle> m_ParticlePool;
		uint32_t m_MaxParticles = 10000;
		uint32_t m_ParticleIndex = 9999; // Start at the end of the pool so the first particle will be at index 0
	};

}
