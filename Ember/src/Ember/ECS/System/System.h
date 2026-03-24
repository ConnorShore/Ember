#pragma once

#include "Ember/Core/Core.h"

namespace Ember {

	class Scene;

	class System : public SharedResource
	{
	public:
		virtual ~System() = default;

		virtual void OnAttach() = 0;
		virtual void OnDetach() = 0;
		virtual void OnUpdate(TimeStep delta, Scene* scene) = 0;
	};

}