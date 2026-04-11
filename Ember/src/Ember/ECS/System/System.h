#pragma once

#include "Ember/Core/Core.h"

namespace Ember {

	class Scene;

	class System : public SharedResource
	{
	public:
		virtual ~System() = default;

		virtual void OnAttach() {}
		virtual void OnDetach() {}

		virtual void OnSceneAttach(Scene* scene) {}
		virtual void OnSceneDetach(Scene* scene) {}

		virtual void OnUpdate(TimeStep delta, Scene* scene) = 0;
	};

}