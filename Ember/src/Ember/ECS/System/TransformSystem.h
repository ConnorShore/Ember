#pragma once

#include "System.h"
#include "Ember/ECS/Types.h"
#include "Ember/ECS/Registry.h"
#include "Ember/Math/Math.h"

namespace Ember {

	class TransformSystem : public System
	{
	public:
		TransformSystem() = default;
		virtual ~TransformSystem() = default;

		void OnAttach(Registry* registry) override;
		void OnDetach(Registry* registry) override;
		void OnUpdate(TimeStep delta, Registry* registry) override;

	private:
		void UpdateTransformTree(EntityID entity, const Matrix4f& parentWorldTransform, Registry* registry);
	};

}