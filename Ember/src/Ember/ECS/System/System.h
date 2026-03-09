#pragma once

#include "Ember/Core/Core.h"

namespace Ember {

	class Registry;

	class System : public SharedResource
	{
	public:
		virtual ~System() = default;

		virtual void OnAttach(Registry* registry) = 0;
		virtual void OnDetach(Registry* registry) = 0;
		virtual void OnUpdate(TimeStep delta, Registry* registry) = 0;
	};

}