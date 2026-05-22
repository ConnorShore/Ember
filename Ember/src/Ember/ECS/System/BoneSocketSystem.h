#pragma once

#include "System.h"
#include "Ember/ECS/Types.h"
#include "Ember/Math/Math.h"

namespace Ember {

	struct BoneSocketComponent;

	class BoneSocketSystem : public System
	{
	public:
		BoneSocketSystem() = default;
		virtual ~BoneSocketSystem() = default;

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate(TimeStep delta, Scene* scene) override;

		static bool SetOffsetFromWorldTransform(BoneSocketComponent& socket, const Matrix4f& worldTransform, Scene* scene);

	private:
		void UpdateChildTransformTree(EntityID entity, const Matrix4f& parentWorldTransform, Scene* scene);
	};

}