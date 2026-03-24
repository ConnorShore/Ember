#pragma once

#include "System.h"

namespace Ember {

	class Scene;

	class ScriptSystem : public System
	{
	public:
		ScriptSystem() = default;
		virtual ~ScriptSystem() = default;

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate(TimeStep delta, Scene* scene) override;
	};

}