#pragma once

#include "System.h"

#include "Ember/Render/ParticleManager.h"

namespace Ember {

	class ParticleSystem : public System
	{
	public:
		ParticleSystem(uint32_t maxParticles = 10000);
		virtual ~ParticleSystem() = default;

		virtual void OnUpdate(TimeStep delta, Scene* scene) override;

		ParticleManager& GetParticleManager() const { return *m_ParticleManager.Ptr(); }
	private:
		ScopedPtr<ParticleManager> m_ParticleManager;
	};
}