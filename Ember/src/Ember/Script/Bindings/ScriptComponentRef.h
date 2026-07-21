#pragma once

#include "Ember/Scene/Scene.h"
#include "Ember/Scene/Entity.h"
#include "Ember/ECS/Component/Components.h"

#include <sol/sol.hpp>
#include <stdexcept>
#include <utility>

namespace Ember {

	// A stable, cacheable Lua handle to a component.
	//
	// It stores the owning Entity (EntityID + Scene*, both stable for the entity's lifetime)
	// instead of a raw pointer into the packed component array, so scripts may safely keep it
	// across frames (e.g. cache it once in OnCreate). Component storage is a swap-and-pop
	// packed vector, so a raw component pointer dangles the moment any entity of that type is
	// spawned (realloc) or destroyed (swap) — this handle avoids that by re-resolving the live
	// component through the sparse set (an O(1) lookup) on every access.
	template<typename T>
	struct ComponentRef
	{
		Entity Owner;

		// Resolves the live component. Throws if the entity or component no longer exists;
		// because scripts are invoked through sol::protected_function, that surfaces as a
		// clean, logged Lua error instead of the use-after-free a stale raw pointer would give.
		T& Resolve() const
		{
			Entity owner = Owner;
			if (owner.IsValid() && owner.ContainsComponent<T>())
				return owner.GetComponent<T>();

			EB_CORE_ASSERT(false, "Used a component handle whose entity or component no longer exists (was it destroyed?).");
			throw std::runtime_error("Used a component handle whose entity or component no longer exists (was it destroyed?).");
		}
	};

	// Binds a plain data member as a resolving property. The getter returns a reference so both
	// whole-field assignment (transform.Position = v) and nested writes (transform.Rotation.y = x)
	// behave exactly like the previous raw member-pointer bindings.
	template<typename T, typename M>
	inline auto RefProp(M T::* member)
	{
		return sol::property(
			[member](ComponentRef<T>& r) -> M& { return r.Resolve().*member; },
			[member](ComponentRef<T>& r, const M& value) { r.Resolve().*member = value; });
	}

	// Binds a member function that resolves the live component before forwarding the call.
	// Two overloads so both const and non-const methods bind.
	template<typename T, typename R, typename... Args>
	inline auto RefMethod(R (T::* fn)(Args...))
	{
		return [fn](ComponentRef<T>& r, Args... args) -> R { return (r.Resolve().*fn)(std::forward<Args>(args)...); };
	}

	template<typename T, typename R, typename... Args>
	inline auto RefMethod(R (T::* fn)(Args...) const)
	{
		return [fn](ComponentRef<T>& r, Args... args) -> R { return (r.Resolve().*fn)(std::forward<Args>(args)...); };
	}

	// The single point where a component is handed to Lua. Every script-exposed component is
	// bound as a resolving ComponentRef handle (see the new_usertype<ComponentRef<T>> registrations
	// in ScriptBindComponents*.cpp), so scripts may safely cache what GetComponent/AttachComponent
	// return. Both of those route through here.
	template<typename T>
	inline sol::object PushComponent(sol::state& state, Entity entity)
	{
		return sol::make_object(state, ComponentRef<T>{ entity });
	}
}
