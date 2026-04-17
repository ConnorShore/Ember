#pragma once

#include "System.h"

#include "Ember/Math/Math.h"

namespace Ember {

	class Entity;
	class PhysicsSystem;

	class CharacterControllerSystem : public System
	{
	public:
		CharacterControllerSystem() = default;
		virtual ~CharacterControllerSystem() = default;

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate(TimeStep delta, Scene* scene) override;

		void Move(Entity entity, const Vector3f& displacement, const SharedPtr<PhysicsSystem>& physicsSystem);
	};

}