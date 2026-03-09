#pragma once

#include "System.h"
#include "Ember/ECS/Registry.h"

namespace Ember {

	class PhysicsSystem : public System
	{
	public:
		PhysicsSystem() = default;
		virtual ~PhysicsSystem() = default;

		void OnAttach(Registry* registry) override;
		void OnDetach(Registry* registry) override;
		void OnUpdate(TimeStep delta, Registry* registry) override;
	};

}