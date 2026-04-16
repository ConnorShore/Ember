#pragma once

#include "System.h"
#include "Ember/ECS/Types.h"
#include "Ember/ECS/Registry.h"
#include "Ember/Math/Math.h"

namespace Ember {

	class Scene;

	class TransformSystem : public System
	{
	public:
		TransformSystem() = default;
		virtual ~TransformSystem() = default;

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate(TimeStep delta, Scene* scene) override;
		void UpdateTransformTree(EntityID entity, const Matrix4f& parentWorldTransform, Scene* scene);
	};

}