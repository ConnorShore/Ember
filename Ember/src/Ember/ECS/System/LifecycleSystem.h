#pragma once

#include "System.h"

namespace Ember {

	class LifecycleSystem : public System
	{
	public:
		LifecycleSystem() = default;
		virtual ~LifecycleSystem() = default;

		virtual void OnUpdate(TimeStep delta, Scene* scene) override;
	};

}