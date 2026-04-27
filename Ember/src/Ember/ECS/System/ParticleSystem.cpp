#include "ebpch.h"
#include "ParticleSystem.h"

#include "Ember/Scene/Scene.h"

namespace Ember {

	ParticleSystem::ParticleSystem(uint32_t maxParticles /* = 10000*/)
		: m_ParticleManager(ScopedPtr<ParticleManager>::Create(maxParticles))
	{
	}

	void ParticleSystem::OnUpdate(TimeStep delta, Scene* scene)
	{
		auto& registry = scene->GetRegistry();
		auto view = registry.ActiveQuery<TransformComponent, ParticleEmitterComponent>();

		for (auto entity : view)
		{
			auto [transform, emitter] = registry.GetComponents<TransformComponent, ParticleEmitterComponent>(entity);

			if (!emitter.IsActive)
			{
				emitter.EmissionAccumulator = 0.0f;
				continue;
			}

			// Calculate how many particles to spawn this frame based on DeltaTime
			emitter.EmissionAccumulator += emitter.EmissionRate * delta;

			while (emitter.EmissionAccumulator > 1.0f)
			{
				m_ParticleManager->Emit(emitter, transform.Position);
				emitter.EmissionAccumulator -= 1.0f;
			}
		}

		// Now that all emitters have fired, advance the physics simulation!
		m_ParticleManager->OnUpdate(delta);
	}

}