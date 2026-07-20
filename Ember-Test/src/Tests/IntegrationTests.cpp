// INTEGRATION TESTS
// -----------------
// Multiple engine subsystems wired together, but no rendering. These need a live Application (asset
// manager + systems), which the runner guarantees is up before invoking them. Slower than unit tests —
// keep them focused on the *seams* between systems (ECS <-> systems, serialization round-trips, ...).

#include <Ember.h>
#include "TestFramework.h"

#include <chrono>
#include <filesystem>
#include <string>

using namespace Ember;
using Ember::Test::Type::Integration;
using Ember::Test::Type::Performance;

// --- ECS: component lifecycle + containment through the real Scene / Registry ---
EB_TEST_CASE(Ecs, AttachDetachContains, Integration)
{
	Scene scene("EcsTestScene", "");

	Entity a = scene.AddEntity("A");
	Entity b = scene.AddEntity("B");
	a.AttachComponent<DirectionalLightComponent>();

	EB_CHECK(a.ContainsComponent<DirectionalLightComponent>());
	EB_CHECK_FALSE(b.ContainsComponent<DirectionalLightComponent>());

	// Default comes from the component's member initializer — verifies real construction, not just storage.
	EB_CHECK_NEAR(a.GetComponent<DirectionalLightComponent>().Intensity, 5.0f, 1e-5);

	a.DetachComponent<DirectionalLightComponent>();
	EB_CHECK_FALSE(a.ContainsComponent<DirectionalLightComponent>());
}

// --- TransformSystem: local TRS -> WorldTransform (the seam between ECS data and a system) ---
EB_TEST_CASE(Transform, WorldTransformFromLocal, Integration)
{
	Scene scene("TransformTestScene", "");
	Entity e = scene.AddEntity("Mover");
	e.AttachComponent<TransformComponent>(Vector3f(1.0f, 2.0f, 3.0f), Vector3f(0.0f), Vector3f(1.0f));

	Application::Instance().GetSystem<TransformSystem>()->OnUpdate(TimeStep(1.0f / 60.0f), &scene);

	const Vector3f world = e.GetComponent<TransformComponent>().GetWorldPosition();
	EB_CHECK_NEAR(world.x, 1.0f, 1e-4);
	EB_CHECK_NEAR(world.y, 2.0f, 1e-4);
	EB_CHECK_NEAR(world.z, 3.0f, 1e-4);
}

// --- Serialization: the exact class of path where the skybox-intensity bug hid ---
// Identity round-trips by UUID (the stable identity); the by-name lookup is checked separately so a
// tag/name-lookup nuance is pinpointed rather than masking the (more important) data round-trip.
EB_TEST_CASE(Serialization, SceneEntityRoundTrip, Integration)
{
	std::filesystem::create_directories("Ember-Test/tmp");
	const std::string path = "Ember-Test/tmp/roundtrip.ebs";

	UUID savedUUID;
	{
		auto scene = SharedPtr<Scene>::Create("RoundTripScene", "");
		Entity e = scene->AddEntity("Persisted");
		e.AttachComponent<TransformComponent>(Vector3f(7.0f, 0.0f, 0.0f), Vector3f(0.0f), Vector3f(1.0f));
		savedUUID = e.GetUUID();

		SceneSerializer serializer(scene);
		serializer.Serialize(path);
		EB_CHECK_MSG(std::filesystem::exists(path), "scene file was not written");
	}

	{
		auto loaded = SharedPtr<Scene>::Create("Loaded", "");
		SceneSerializer serializer(loaded);
		EB_CHECK(serializer.Deserialize(path));

		// Print EVERYTHING before asserting, so a failure still yields full diagnostics.
		auto all = loaded->GetAllEntities();
		std::printf("         (saved uuid=%llu ; loaded %zu entities)\n",
			(unsigned long long)(uint64_t)savedUUID, all.size());

		Entity found;
		for (Entity ent : all)
		{
			const UUID u = ent.GetUUID();
			const bool eq = (u == savedUUID);
			std::printf("           - uuid=%llu name='%s'  (==saved:%d)\n",
				(unsigned long long)(uint64_t)u, ent.GetName().c_str(), (int)eq);
			if (eq) found = ent;
		}

		Entity byId = loaded->GetEntity(savedUUID);
		Entity byName = loaded->GetEntity("Persisted");
		std::printf("         (lookups: enumMatch=%d  find(uuid)=%d  find(name)=%d)\n",
			(int)static_cast<bool>(found), (int)static_cast<bool>(byId), (int)static_cast<bool>(byName));

		// Now the actual assertions.
		EB_CHECK_MSG(found == byId, "entity did not survive round trip [id]");
		EB_CHECK_MSG(found == byName, "entity did not survive round trip [name]");
		EB_CHECK(found.ContainsComponent<TransformComponent>());
		EB_CHECK_NEAR(found.GetComponent<TransformComponent>().Position.x, 7.0f, 1e-4);
	}
}

// --- PERFORMANCE (demonstration) ---
// Perf tests assert a task stays within a time budget. They're inherently machine-dependent: keep budgets
// generous, run on controlled hardware for real numbers, and treat a failure as "investigate," not
// "broken." Shown here as a pattern to copy (a benchmarking lib like nanobench is a good upgrade later).
EB_TEST_CASE(Perf, TransformUpdateManyEntities, Performance)
{
	constexpr int kEntities = 1000; // keep well under Constants::Entities::MaxEntities

	Scene scene("PerfTestScene", "");
	for (int i = 0; i < kEntities; ++i)
	{
		Entity e = scene.AddEntity();
		e.AttachComponent<TransformComponent>(Vector3f((float)i, 0.0f, 0.0f), Vector3f(0.0f), Vector3f(1.0f));
	}

	const auto start = std::chrono::high_resolution_clock::now();
	Application::Instance().GetSystem<TransformSystem>()->OnUpdate(TimeStep(1.0f / 60.0f), &scene);
	const auto end = std::chrono::high_resolution_clock::now();

	const double ms = std::chrono::duration<double, std::milli>(end - start).count();
	std::printf("         (TransformUpdate %d entities took %.3f ms)\n", kEntities, ms);
	EB_CHECK_MSG(ms < 50.0, "transform update took " + std::to_string(ms) + " ms (budget 50 ms)");
}
