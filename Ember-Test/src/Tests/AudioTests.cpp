// AudioSourceComponent is move-only and owns a raw ma_sound*, which collides with the ECS relocating
// components on growth and removal. These tests pin that interaction down.

#include <Ember.h>

#include "TestFramework.h"
#include "TestHelpers.h"

#include <string>
#include <vector>

using namespace Ember;
using Ember::Test::Type::Integration;
using Ember::Test::MakeEntityAt;
using Ember::Test::SceneFixture;
using Ember::Test::Sys;

//////////////////////////////////////////////////////////////////////////
// System availability
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Audio, EngineIsInitialised, Integration)
{
	auto audioSystem = Sys<AudioSystem>();
	EB_CHECK_MSG(audioSystem != nullptr, "AudioSystem was never registered with the SystemManager");

	if (audioSystem->GetAudioEngine() == nullptr)
		EB_SKIP("no audio device available (expected on a headless machine)");

	EB_EXPECT(audioSystem->GetAudioEngine() != nullptr);
}

//////////////////////////////////////////////////////////////////////////
// Component defaults
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Audio, SourceComponentDefaults, Integration)
{
	SceneFixture scene("AudioDefaultsScene");
	Entity entity = MakeEntityAt(*scene, "Speaker", Vector3f(0.0f));

	auto& source = entity.AttachComponent<AudioSourceComponent>();

	EB_EXPECT_EQ(source.AudioClipHandle, UUID(Constants::InvalidUUID));
	EB_EXPECT_FALSE(source.PlayOnStart);
	EB_EXPECT_MSG(!source.Source.IsLoaded(), "a fresh AudioSourceComponent reports a loaded sound");
	EB_EXPECT_MSG(source.Source.GetSound() == nullptr, "a fresh AudioSourceComponent already owns an ma_sound");
	EB_EXPECT_FALSE(source.Source.IsPlaying.load());

	// Sound property defaults must be audible-and-neutral, not zeroed.
	EB_EXPECT_NEAR(source.Properties.Volume, 1.0f, 1e-6);
	EB_EXPECT_NEAR(source.Properties.Pitch, 1.0f, 1e-6);
	EB_EXPECT_FALSE(source.Properties.Looping);
	EB_EXPECT_FALSE(source.Properties.Spatialized);
	EB_EXPECT_LT(source.Properties.MinDistance, source.Properties.MaxDistance);
}

EB_TEST_CASE(Audio, ListenerComponentDefaults, Integration)
{
	SceneFixture scene("AudioListenerScene");
	Entity entity = MakeEntityAt(*scene, "Listener", Vector3f(0.0f));

	auto& listener = entity.AttachComponent<AudioListenerComponent>();
	EB_EXPECT_EQ(listener.ListenerIndex, (uint32_t)0);
	EB_EXPECT(listener.IsActive);
}

//////////////////////////////////////////////////////////////////////////
// Move-only component storage
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Audio, SourceComponentsSurviveDenseArrayGrowth, Integration)
{
	// Growing the dense array reallocates it and MOVES every existing component. Each source must
	// come out the other side owning its own state - if the move constructor ever failed to null the
	// donor, this is where the double free would be seeded.
	SceneFixture scene("AudioGrowthScene");

	constexpr int kCount = 40;
	std::vector<Entity> entities;
	for (int i = 0; i < kCount; ++i)
	{
		Entity entity = MakeEntityAt(*scene, "Speaker" + std::to_string(i), Vector3f((float)i, 0.0f, 0.0f));
		auto& source = entity.AttachComponent<AudioSourceComponent>();
		// Tag each component so a mis-mapped move is visible rather than silent.
		source.AudioClipHandle = UUID((uint64_t)(1000 + i));
		source.Properties.Volume = 0.1f * (float)i;
		entities.push_back(entity);
	}

	for (int i = 0; i < kCount; ++i)
	{
		EB_EXPECT_MSG(entities[i].ContainsComponent<AudioSourceComponent>(),
			"entity " + std::to_string(i) + " lost its AudioSourceComponent during array growth");

		auto& source = entities[i].GetComponent<AudioSourceComponent>();
		EB_EXPECT_MSG((uint64_t)source.AudioClipHandle == (uint64_t)(1000 + i),
			"entity " + std::to_string(i) + " resolved to another entity's audio source");
		EB_EXPECT_NEAR(source.Properties.Volume, 0.1f * (float)i, 1e-4);
	}
}

EB_TEST_CASE(Audio, SourceComponentsSurviveSwapAndPopRemoval, Integration)
{
	// Detaching moves the LAST component into the freed slot. For a move-only type carrying a raw
	// pointer that has to be exactly right, or the surviving entity ends up pointing at a sound the
	// removed one still believes it owns.
	SceneFixture scene("AudioRemovalScene");

	constexpr int kCount = 12;
	std::vector<Entity> entities;
	for (int i = 0; i < kCount; ++i)
	{
		Entity entity = MakeEntityAt(*scene, "Speaker" + std::to_string(i), Vector3f(0.0f));
		auto& source = entity.AttachComponent<AudioSourceComponent>();
		source.AudioClipHandle = UUID((uint64_t)(2000 + i));
		entities.push_back(entity);
	}

	// Remove from the middle: index 4 gets the last component swapped into it.
	entities[4].DetachComponent<AudioSourceComponent>();
	entities[0].DetachComponent<AudioSourceComponent>();

	EB_EXPECT_FALSE(entities[4].ContainsComponent<AudioSourceComponent>());
	EB_EXPECT_FALSE(entities[0].ContainsComponent<AudioSourceComponent>());

	for (int i = 0; i < kCount; ++i)
	{
		if (i == 0 || i == 4)
			continue;

		EB_EXPECT_MSG(entities[i].ContainsComponent<AudioSourceComponent>(),
			"entity " + std::to_string(i) + " lost its AudioSourceComponent during an unrelated removal");

		auto& source = entities[i].GetComponent<AudioSourceComponent>();
		EB_EXPECT_MSG((uint64_t)source.AudioClipHandle == (uint64_t)(2000 + i),
			"entity " + std::to_string(i) + " resolved to another entity's audio source after removal");
	}
}

EB_TEST_CASE(Audio, DestroyingEntitiesReleasesTheirSources, Integration)
{
	// The scene-teardown path. Nothing to assert beyond "this does not crash or corrupt the heap",
	// which is precisely the failure mode a move-only component with a raw pointer produces.
	SceneFixture scene("AudioTeardownScene");

	for (int i = 0; i < 20; ++i)
	{
		Entity entity = MakeEntityAt(*scene, "Speaker" + std::to_string(i), Vector3f(0.0f));
		entity.AttachComponent<AudioSourceComponent>().AudioClipHandle = UUID((uint64_t)(3000 + i));
	}

	EB_CHECK_EQ(scene->GetAllEntities().size(), (size_t)20);

	scene->Clear();
	EB_EXPECT_EQ(scene->GetAllEntities().size(), (size_t)0);

	// The scene is reusable afterwards.
	Entity fresh = MakeEntityAt(*scene, "AfterClear", Vector3f(0.0f));
	fresh.AttachComponent<AudioSourceComponent>();
	EB_EXPECT(fresh.ContainsComponent<AudioSourceComponent>());
}

//////////////////////////////////////////////////////////////////////////
// System update
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Audio, SystemUpdateToleratesSourcesWithNoClip, Integration)
{
	// A component whose clip has not been assigned yet is a normal authoring state. The system must
	// skip it, not dereference a null clip handle.
	auto audioSystem = Sys<AudioSystem>();
	EB_CHECK(audioSystem != nullptr);

	SceneFixture scene("AudioUpdateScene");
	Entity speaker = MakeEntityAt(*scene, "Speaker", Vector3f(1.0f, 0.0f, 0.0f));
	speaker.AttachComponent<AudioSourceComponent>();

	Entity listener = MakeEntityAt(*scene, "Listener", Vector3f(0.0f));
	listener.AttachComponent<AudioListenerComponent>();

	scene.UpdateTransforms();

	// Several ticks, to catch anything that only goes wrong on a repeat pass.
	for (int i = 0; i < 5; ++i)
		audioSystem->OnUpdate(Ember::Test::FixedStep(), scene.Ptr());

	EB_EXPECT(speaker.ContainsComponent<AudioSourceComponent>());
	EB_EXPECT_FALSE(speaker.GetComponent<AudioSourceComponent>().Source.IsPlaying.load());
}
