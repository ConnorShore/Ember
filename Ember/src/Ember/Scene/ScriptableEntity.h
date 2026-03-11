#pragma once

#include "SceneEntity.h"

namespace Ember {

	class ScriptSystem;

	class ScriptableEntity
	{
	public:
		virtual ~ScriptableEntity() = default;

		// API of what can be called by the user
		template<typename T>
		T& GetComponent()
		{
			return m_SceneEntityHandle.GetComponent<T>();
		}

	protected:
		virtual void OnCreate() {}
		virtual void OnUpdate(TimeStep delta) {}
		virtual void OnDestroy() {}

	private:
		SceneEntity m_SceneEntityHandle;

		friend class ScriptSystem;
	};

}