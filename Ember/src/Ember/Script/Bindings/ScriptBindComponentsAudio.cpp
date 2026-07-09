#include "ebpch.h"
#include "ScriptBindComponents.h"
#include "Ember/ECS/Component/Components.h"

namespace Ember {
	void BindAudioComponents(sol::state& state)
	{
		state.new_usertype<AudioSourceComponent>("AudioSourceComponent",
			"AudioClipHandle", &AudioSourceComponent::AudioClipHandle,
			"Volume", sol::property(
				[](AudioSourceComponent& c) { return c.Properties.Volume; },
				[](AudioSourceComponent& c, float volume) { c.Properties.Volume = volume; }
			),
			"Pitch", sol::property(
				[](AudioSourceComponent& c) { return c.Properties.Pitch; },
				[](AudioSourceComponent& c, float pitch) { c.Properties.Pitch = pitch; }
			),
			"Looping", sol::property(
				[](AudioSourceComponent& c) { return c.Properties.Looping; },
				[](AudioSourceComponent& c, bool looping) { c.Properties.Looping = looping; }
			),
			"Spatialized", sol::property(
				[](AudioSourceComponent& c) { return c.Properties.Spatialized; },
				[](AudioSourceComponent& c, bool spatialized) { c.Properties.Spatialized = spatialized; }
			),
			"Play", sol::as_function(
				[](AudioSourceComponent& c) { c.Source.Play(); }
			),
			"PlayDelayed", sol::as_function(
				[](AudioSourceComponent& c, float delayMs) { c.Source.PlayDelayed(delayMs); }
			),
			"Stop", sol::as_function(
				[](AudioSourceComponent& c) { c.Source.Stop(); }
			),
			"Restart", sol::as_function(
				[](AudioSourceComponent& c) { c.Source.Restart(); }
			)
		);

		state.new_usertype<AudioListenerComponent>("AudioListenerComponent",
			"IsActive", &AudioListenerComponent::IsActive,
			"ListenerIndex", &AudioListenerComponent::ListenerIndex
		);
	}
}