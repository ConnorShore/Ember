#include "ebpch.h"
#include "ScriptBindPhysics.h"

#include "Ember/Physics/CollisionFilterManager.h"

namespace Ember {

	void BindPhysics(sol::state& state)
	{
		state.new_usertype<CollisionFilterManager>("CollisionFilterManager",
			"AddFilter", &CollisionFilterManager::AddFilter,
			"GetFilter", &CollisionFilterManager::GetFilter
		);
	}

}
