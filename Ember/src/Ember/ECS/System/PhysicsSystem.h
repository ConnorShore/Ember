#pragma once

#include "System.h"

namespace Ember {

	class PhysicsSystem : public System
	{
	public:
		PhysicsSystem() = default;
		virtual ~PhysicsSystem() = default;

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate(TimeStep delta, Scene* scene) override;
	};

}