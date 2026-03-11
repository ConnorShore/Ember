#pragma once

#include "Entity.h"

namespace Ember {

	class ScriptSystem;

	class Behavior
	{
	public:
		virtual ~Behavior() = default;

		// API of what can be called by the user
		template<typename T>
		T& GetComponent() { return m_EntityHandle.GetComponent<T>(); }

		// Standard Component Getters
		TransformComponent& Transform() { return GetComponent<TransformComponent>(); }
		const std::string& Tag() { return GetComponent<TagComponent>().Tag; }

	protected:
		virtual void OnCreate() {}
		virtual void OnUpdate(TimeStep delta) {}
		virtual void OnDestroy() {}

	private:
		Entity m_EntityHandle;

		friend class ScriptSystem;
	};

}