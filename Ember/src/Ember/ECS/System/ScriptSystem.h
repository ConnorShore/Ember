#pragma once

#include "System.h"

namespace Ember {

	class Scene;

	class ScriptSystem : public System
	{
	public:
		ScriptSystem(Scene* scene) : m_SceneHandle(scene) {}
		virtual ~ScriptSystem() = default;

		void OnAttach(Registry* registry) override;
		void OnDetach(Registry* registry) override;
		void OnUpdate(TimeStep delta, Registry* registry) override;

	private:
		Scene* m_SceneHandle;
	};

}