#pragma once

#include "System.h"

#include "Ember/Render/ParticleManager.h"

namespace Ember {

	class ParticleSystem : public System
	{
	public:
		ParticleSystem() = default;
		virtual ~ParticleSystem() = default;

		virtual void OnUpdate(TimeStep delta, Scene* scene) override;
	};
}
