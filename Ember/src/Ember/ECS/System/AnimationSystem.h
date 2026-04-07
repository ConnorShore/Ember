#pragma once

#include "System.h"

namespace Ember {

	class AnimationSystem : public System
	{
	public:
		AnimationSystem() = default;
		virtual ~AnimationSystem() = default;

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate(TimeStep delta, Scene* scene) override;
	};

}