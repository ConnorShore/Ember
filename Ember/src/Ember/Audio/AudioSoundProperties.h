#pragma once

namespace Ember {

	struct AudioSoundProperties
	{
		float Volume = 1.0f;
		float Pitch = 1.0f;
		bool Looping = false;

		bool Spatialized = false;
		float MinDistance = 0.0f; // For spatialized sounds, the distance at which the sound is heard at full volume
		float MaxDistance = 10.0f; // For spatialized sounds, the distance beyond which the sound is no longer audible
	};

}