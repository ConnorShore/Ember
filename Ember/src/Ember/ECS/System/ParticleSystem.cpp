#include "ebpch.h"
#include "ParticleSystem.h"

#include "Ember/Scene/Scene.h"
#include "Ember/Asset/AssetManager.h"

namespace Ember {

	void ParticleSystem::OnUpdate(TimeStep delta, Scene* scene)
	{
		auto& registry = scene->GetRegistry();
		auto& particleManager = scene->GetParticleManager();
		auto& assetManager = Application::Instance().GetAssetManager();

		auto view = registry.ActiveQuery<TransformComponent, ParticleEmitterComponent>();

		for (auto entity : view)
		{
			auto [transform, emitter] = registry.GetComponents<TransformComponent, ParticleEmitterComponent>(entity);

			if (!emitter.IsActive)
			{
				emitter.EmissionAccumulator = 0.0f;
				continue;
			}

			auto effect = assetManager.GetAsset<ParticleEffect>(emitter.EffectHandle);
			if (!effect)
			{
				emitter.EmissionAccumulator = 0.0f;
				continue;
			}

			const auto& properties = effect->Properties;

			// Calculate how many particles to spawn this frame based on DeltaTime
			emitter.EmissionAccumulator += properties.EmissionRate * delta;

			// Decompose world rotation so particles are emitted in the emitter's local space
			Vector3f worldPos, worldRot, worldScale;
			Math::DecomposeTransform(transform.WorldTransform, worldPos, worldRot, worldScale);
			Quaternion worldRotation = Math::ToQuaternion(worldRot);

			while (emitter.EmissionAccumulator > 1.0f)
			{
				particleManager.Emit(properties, transform.GetWorldPosition(), worldRotation);
				emitter.EmissionAccumulator -= 1.0f;
			}
		}

		// Now that all emitters have fired, advance the physics simulation!
		particleManager.OnUpdate(delta);
	}

}
