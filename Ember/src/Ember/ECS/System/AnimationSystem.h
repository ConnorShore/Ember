#pragma once

#include "System.h"

namespace Ember {

	class Entity;

	class AnimationSystem : public System
	{
	public:
		AnimationSystem() = default;
		virtual ~AnimationSystem() = default;

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate(TimeStep delta, Scene* scene) override;

		void SetAnimationToTimestamp(Scene* scene, UUID animationHandle, Entity entity, float timestamp);
	};

}