#include "ebpch.h"
#include "ScriptBindPhysics.h"

#include "Ember/Physics/CollisionFilterManager.h"

namespace Ember {

	void BindPhysics(sol::state& state)
	{
		state.new_usertype<CollisionFilterManager>("CollisionFilterManager",
			"SetFilterNameAtSlot", &CollisionFilterManager::SetFilterNameAtSlot,
			"GetFilter", &CollisionFilterManager::GetFilter,
			"GetFilterNameBySlot", &CollisionFilterManager::GetFilterNameBySlot
		);
	}

}
