#include "ebpch.h"
#include "ScriptBindComponents.h"
#include "Ember/ECS/Component/Components.h"
#include "Ember/Script/Bindings/ScriptComponentRef.h"

namespace Ember {
	void BindAudioComponents(sol::state& state)
	{
		// Bound as resolving handles (see ScriptComponentRef.h): safe to cache in Lua.
		state.new_usertype<ComponentRef<AudioSourceComponent>>("AudioSourceComponent",
			"AudioClipHandle", RefProp(&AudioSourceComponent::AudioClipHandle),
			"Volume", sol::property(
				[](ComponentRef<AudioSourceComponent>& r) { return r.Resolve().Properties.Volume; },
				[](ComponentRef<AudioSourceComponent>& r, float volume) { r.Resolve().Properties.Volume = volume; }
			),
			"Pitch", sol::property(
				[](ComponentRef<AudioSourceComponent>& r) { return r.Resolve().Properties.Pitch; },
				[](ComponentRef<AudioSourceComponent>& r, float pitch) { r.Resolve().Properties.Pitch = pitch; }
			),
			"Looping", sol::property(
				[](ComponentRef<AudioSourceComponent>& r) { return r.Resolve().Properties.Looping; },
				[](ComponentRef<AudioSourceComponent>& r, bool looping) { r.Resolve().Properties.Looping = looping; }
			),
			"Spatialized", sol::property(
				[](ComponentRef<AudioSourceComponent>& r) { return r.Resolve().Properties.Spatialized; },
				[](ComponentRef<AudioSourceComponent>& r, bool spatialized) { r.Resolve().Properties.Spatialized = spatialized; }
			),
			"Play", sol::as_function(
				[](ComponentRef<AudioSourceComponent>& r) { r.Resolve().Source.Play(); }
			),
			"PlayDelayed", sol::as_function(
				[](ComponentRef<AudioSourceComponent>& r, float delayMs) { r.Resolve().Source.PlayDelayed(delayMs); }
			),
			"Stop", sol::as_function(
				[](ComponentRef<AudioSourceComponent>& r) { r.Resolve().Source.Stop(); }
			),
			"Restart", sol::as_function(
				[](ComponentRef<AudioSourceComponent>& r) { r.Resolve().Source.Restart(); }
			)
		);

		state.new_usertype<ComponentRef<AudioListenerComponent>>("AudioListenerComponent",
			"IsActive", RefProp(&AudioListenerComponent::IsActive),
			"ListenerIndex", RefProp(&AudioListenerComponent::ListenerIndex)
		);
	}
}