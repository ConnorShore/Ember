// Absolute budgets are loose and hardware-dependent - they catch order-of-magnitude regressions, not
// a few percent, and are scaled by EMBER_TEST_PERF_SCALE. Relative comparisons (X must be cheaper
// than Y) are the robust ones and the only way to prove an optimisation still engages.

#include <Ember.h>

// VisibilitySystem is not re-exported from Ember.h, unlike the other systems.
#include "Ember/ECS/System/VisibilitySystem.h"

#include "TestFramework.h"
#include "TestHelpers.h"

#include <filesystem>
#include <string>
#include <vector>

using namespace Ember;
using Ember::Test::Type::Performance;
using Ember::Test::Benchmark;
using Ember::Test::BenchmarkResult;
using Ember::Test::MakeDynamicBox;
using Ember::Test::MakeEntityAt;
using Ember::Test::MakeStaticBox;
using Ember::Test::PhysicsSceneFixture;
using Ember::Test::SceneFixture;
using Ember::Test::Sys;

namespace {

	// Comfortably under Constants::Entities::MaxEntities (1024) with headroom for the fixtures'
	// own entities.
	constexpr int kManyEntities = 900;

} // namespace

//////////////////////////////////////////////////////////////////////////
// Transform system
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Perf, TransformFlatSceneUpdate, Performance)
{
	SceneFixture scene("PerfFlatTransformScene");
	for (int i = 0; i < kManyEntities; ++i)
		MakeEntityAt(*scene, "", Vector3f((float)i, 0.0f, 0.0f));

	auto transforms = Sys<TransformSystem>();
	Scene* scenePtr = scene.Ptr();

	// Worst case: every entity dirty every frame, so every world matrix is rebuilt.
	EB_BENCH_BUDGET("flat scene, all dirty", 6.0, 30, {
		auto view = scenePtr->GetRegistry().Query<TransformComponent>();
		for (EntityID entity : view)
			scenePtr->GetRegistry().GetComponent<TransformComponent>(entity).Position.x += 0.001f;
		transforms->OnUpdate(Ember::Test::FixedStep(), scenePtr);
		});
}

EB_TEST_CASE(Perf, TransformDirtyFlagActuallySkipsWork, Performance)
{
	// THE test for the transform dirty-flag optimisation. A static scene must cost dramatically less
	// than one where everything moved. If this ever regresses to parity the flag has stopped
	// engaging - which produces no wrong output at all, just a permanently slower frame.
	SceneFixture scene("PerfDirtyFlagScene");
	for (int i = 0; i < kManyEntities; ++i)
		MakeEntityAt(*scene, "", Vector3f((float)i, 0.0f, 0.0f));

	auto transforms = Sys<TransformSystem>();
	Scene* scenePtr = scene.Ptr();

	// Settle everything so the first measured pass starts clean.
	transforms->OnUpdate(Ember::Test::FixedStep(), scenePtr);

	const BenchmarkResult clean = Benchmark([&]() {
		transforms->OnUpdate(Ember::Test::FixedStep(), scenePtr);
		}, 50);

	const BenchmarkResult dirty = Benchmark([&]() {
		auto view = scenePtr->GetRegistry().Query<TransformComponent>();
		for (EntityID entity : view)
			scenePtr->GetRegistry().GetComponent<TransformComponent>(entity).Position.x += 0.001f;
		transforms->OnUpdate(Ember::Test::FixedStep(), scenePtr);
		}, 50);

	EB_NOTE("clean pass: " + clean.ToString());
	EB_NOTE("dirty pass: " + dirty.ToString());

	// Deliberately lenient (10% rather than the several-fold speedup the flag really gives) so this
	// never fails on timer noise - it is here to catch the flag being bypassed entirely.
	EB_EXPECT_MSG(clean.MedianMs < dirty.MedianMs * 0.9,
		"a fully static transform pass (" + std::to_string(clean.MedianMs)
		+ " ms) is not meaningfully cheaper than a fully dirty one (" + std::to_string(dirty.MedianMs)
		+ " ms) - the dirty-flag fast path is no longer engaging");
}

EB_TEST_CASE(Perf, TransformDeepHierarchyUpdate, Performance)
{
	// Deep chains stress UpdateTransformTree's recursion and the parentChanged propagation, which
	// behave very differently from a flat scene.
	SceneFixture scene("PerfDeepTransformScene");

	constexpr int kChains = 30;
	constexpr int kDepth = 30;
	std::vector<Entity> roots;
	for (int c = 0; c < kChains; ++c)
	{
		Entity current = MakeEntityAt(*scene, "Root" + std::to_string(c), Vector3f(0.0f));
		roots.push_back(current);
		for (int d = 1; d < kDepth; ++d)
		{
			current = current.AddChild("");
			current.GetComponent<TransformComponent>().Position = Vector3f(1.0f, 0.0f, 0.0f);
		}
	}

	auto transforms = Sys<TransformSystem>();
	Scene* scenePtr = scene.Ptr();
	transforms->OnUpdate(Ember::Test::FixedStep(), scenePtr);

	// Moving only the roots must still refresh all 900 nodes via propagation.
	EB_BENCH_BUDGET("deep hierarchy, roots moving", 6.0, 30, {
		for (Entity root : roots)
			root.GetComponent<TransformComponent>().Position.y += 0.001f;
		transforms->OnUpdate(Ember::Test::FixedStep(), scenePtr);
		});
}

//////////////////////////////////////////////////////////////////////////
// ECS
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Perf, QueryIterationThroughput, Performance)
{
	// Views are walked several times per frame by every system, so per-iteration overhead is
	// multiplied by the number of systems. This is the cost that shows up everywhere at once.
	Registry registry;
	for (int i = 0; i < kManyEntities; ++i)
	{
		const EntityID entity = registry.CreateEntity();
		registry.AttachComponent<TransformComponent>(entity, Vector3f((float)i, 0.0f, 0.0f), Vector3f(0.0f), Vector3f(1.0f));
		registry.AttachComponent<PointLightComponent>(entity);
	}

	EB_BENCH_BUDGET("single-component query x900", 1.0, 100, {
		float sum = 0.0f;
		for (EntityID entity : registry.Query<TransformComponent>())
			sum += registry.GetComponent<TransformComponent>(entity).Position.x;
		(void)sum;
		});

	EB_BENCH_BUDGET("two-component query x900", 2.0, 100, {
		float sum = 0.0f;
		for (EntityID entity : registry.Query<TransformComponent, PointLightComponent>())
			sum += registry.GetComponent<PointLightComponent>(entity).Intensity;
		(void)sum;
		});

	EB_BENCH_BUDGET("active query with exclude x900", 2.5, 100, {
		uint64_t sum = 0;
		for (EntityID entity : registry.ActiveQuery<TransformComponent, PointLightComponent>())
			sum += entity;
		(void)sum;
		});
}

EB_TEST_CASE(Perf, EntityChurn, Performance)
{
	// Create-and-destroy is the pattern behind projectiles, particles and pooled spawns. A leak in
	// the free list or a linear scan in the sparse set turns this into a slow bleed over a session.
	EB_BENCH_BUDGET("create+destroy 500 entities with 2 components", 6.0, 20, {
		Registry registry;
		std::vector<EntityID> entities;
		entities.reserve(500);
		for (int i = 0; i < 500; ++i)
		{
			const EntityID entity = registry.CreateEntity();
			registry.AttachComponent<TransformComponent>(entity);
			registry.AttachComponent<LifetimeComponent>(entity, 1.0f);
			entities.push_back(entity);
		}
		for (EntityID entity : entities)
			registry.DestroyEntity(entity);
		});
}

EB_TEST_CASE(Perf, ComponentAttachDetachChurn, Performance)
{
	Registry registry;
	std::vector<EntityID> entities;
	for (int i = 0; i < 500; ++i)
	{
		const EntityID entity = registry.CreateEntity();
		registry.AttachComponent<TransformComponent>(entity);
		entities.push_back(entity);
	}

	// Repeated attach/detach exercises the swap-and-pop path hard - the one place a subtle
	// mis-mapping would also be a correctness bug (see EcsTests).
	EB_BENCH_BUDGET("attach+detach 500 components", 4.0, 30, {
		for (EntityID entity : entities)
			registry.AttachComponent<PointLightComponent>(entity);
		for (EntityID entity : entities)
			registry.DetachComponent<PointLightComponent>(entity);
		});
}

EB_TEST_CASE(Perf, SceneEntityCreation, Performance)
{
	// Scene::AddEntity does more than Registry::CreateEntity: four components plus two lookup-table
	// insertions. It is on the hot path of every spawn and every scene load.
	EB_BENCH_BUDGET("Scene::AddEntity x500", 8.0, 20, {
		SceneFixture scene("PerfSpawnScene");
		for (int i = 0; i < 500; ++i)
			scene->AddEntity("");
		});
}

//////////////////////////////////////////////////////////////////////////
// Physics
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Perf, PhysicsStepWithManyBodies, Performance)
{
	// A frame of simulation with a realistic pile of dynamic bodies. rp3d dominates the time here,
	// so this mostly guards the Ember-side glue: the per-entity transform write-back, the trigger
	// scan and the avoidance pass all run over every body every frame.
	constexpr int kBodies = 150;

	PhysicsSceneFixture scene("PerfPhysicsScene");
	MakeStaticBox(*scene, "Ground", Vector3f(0.0f, -0.5f, 0.0f), Vector3f(200.0f, 1.0f, 200.0f));
	for (int i = 0; i < kBodies; ++i)
	{
		const float x = (float)(i % 12) * 2.0f;
		const float z = (float)(i / 12) * 2.0f;
		MakeDynamicBox(*scene, "Box" + std::to_string(i), Vector3f(x, 5.0f + (float)i * 0.1f, z));
	}
	scene.Attach();

	auto physics = Sys<PhysicsSystem>();
	Scene* scenePtr = scene.Ptr();

	// Let the pile settle so the benchmark measures a steady state with real contacts, which is
	// the expensive case - not a frame of free fall with an empty broadphase.
	scene.Step(120);

	EB_BENCH_BUDGET("physics step, 150 dynamic bodies", 12.0, 30, {
		physics->OnUpdate(Ember::Test::FixedStep(), scenePtr);
		});
}

EB_TEST_CASE(Perf, RaycastThroughput, Performance)
{
	constexpr int kRaysPerBatch = 100;

	PhysicsSceneFixture scene("PerfRaycastScene");
	MakeStaticBox(*scene, "Ground", Vector3f(0.0f, -0.5f, 0.0f), Vector3f(200.0f, 1.0f, 200.0f));
	for (int i = 0; i < 60; ++i)
		MakeStaticBox(*scene, "Block" + std::to_string(i), Vector3f((float)(i % 10) * 3.0f, 1.0f, (float)(i / 10) * 3.0f), Vector3f(1.0f));
	scene.Attach();

	auto physics = Sys<PhysicsSystem>();

	// Scripts fire rays constantly (line of sight, ground checks, weapons), so this is a per-frame
	// cost that scales with the number of agents.
	EB_BENCH_BUDGET("100 downward raycasts", 4.0, 30, {
		int hits = 0;
		for (int i = 0; i < kRaysPerBatch; ++i)
		{
			const float x = (float)(i % 10) * 3.0f;
			const float z = (float)(i / 10) * 3.0f;
			if (physics->CastRay(Vector3f(x, 20.0f, z), Vector3f(0.0f, -1.0f, 0.0f), 50.0f, FilterPreset::All).Hit)
				++hits;
		}
		(void)hits;
		});
}

//////////////////////////////////////////////////////////////////////////
// Scene operations
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Perf, SceneSerializationRoundTrip, Performance)
{
	// Save/load time is felt directly by whoever is using the editor, and it is easy to make
	// quadratic by accident (a name lookup that scans every entity, say).
	constexpr int kEntities = 300;
	const std::string path = Ember::Test::TempFile("perf_scene.ebs");

	SceneFixture source("PerfSerializationScene");
	for (int i = 0; i < kEntities; ++i)
	{
		Entity entity = MakeEntityAt(*source, "Entity" + std::to_string(i), Vector3f((float)i, 0.0f, 0.0f));
		entity.AttachComponent<PointLightComponent>(Vector3f(1.0f), 10.0f, 1.0f);
	}
	source.UpdateTransforms();

	EB_BENCH_BUDGET("serialize 300 entities", 250.0, 10, {
		SceneSerializer serializer(source.Shared());
		serializer.Serialize(path);
		});

	EB_BENCH_BUDGET("deserialize 300 entities", 250.0, 10, {
		SceneFixture target("PerfDeserializationScene");
		SceneSerializer serializer(target.Shared());
		serializer.Deserialize(path);
		});

	Ember::Test::RemoveTempFile(path);
}

EB_TEST_CASE(Perf, SceneCopyForPlayMode, Performance)
{
	// Entering Play mode deep-copies the whole edit scene. This is a visible hitch on the very
	// action a developer performs hundreds of times a day.
	constexpr int kEntities = 300;

	SceneFixture source("PerfCopyScene");
	for (int i = 0; i < kEntities; ++i)
	{
		Entity entity = MakeEntityAt(*source, "Entity" + std::to_string(i), Vector3f((float)i, 0.0f, 0.0f));
		entity.AttachComponent<PointLightComponent>();
	}
	source.UpdateTransforms();

	EB_BENCH_BUDGET("CopyScene of 300 entities", 60.0, 10, {
		SharedPtr<Scene> copy = Scene::CopyScene(source.Shared());
		(void)copy;
		});
}

//////////////////////////////////////////////////////////////////////////
// Culling
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Perf, VisibilityCullThroughput, Performance)
{
	// Visibility runs once per frame over every renderable, before the simulation systems that
	// consume it - so it is pure overhead unless it stays cheap.
	Ember::Test::RequireDefaultAssets();

	constexpr int kRenderables = 400;

	SceneFixture scene("PerfVisibilityScene");
	Entity cameraEntity = MakeEntityAt(*scene, "Camera", Vector3f(0.0f));
	auto& cameraComponent = cameraEntity.AttachComponent<CameraComponent>();
	cameraComponent.IsActive = true;
	cameraComponent.Camera.SetPerspective(60.0f, 0.1f, 500.0f);
	cameraComponent.Camera.SetViewportSize(1280, 720);

	for (int i = 0; i < kRenderables; ++i)
	{
		Entity entity = MakeEntityAt(*scene, "", Vector3f((float)(i % 20) * 4.0f - 40.0f, 0.0f, -(float)(i / 20) * 4.0f - 5.0f));
		entity.AttachComponent<StaticMeshComponent>(UUID(Constants::Assets::CubeMeshUUID));
		entity.AttachComponent<MaterialComponent>(UUID(Constants::Assets::StandardGeometryMatUUID));
	}
	scene.UpdateTransforms();

	auto visibility = Sys<VisibilitySystem>();
	visibility->OnSceneDetach(scene.Ptr());
	Scene* scenePtr = scene.Ptr();

	EB_BENCH_BUDGET("visibility cull, 400 renderables", 6.0, 30, {
		visibility->OnUpdate(Ember::Test::FixedStep(), scenePtr);
		});

	visibility->OnSceneDetach(scene.Ptr());
}

EB_TEST_CASE(Perf, FrustumCullMathThroughput, Performance)
{
	// The inner loop of culling, isolated from any scene traversal or asset lookups. Six plane
	// tests per box; this is the figure to watch if the frustum maths is ever rewritten.
	constexpr int kBoxes = 5000;

	const Matrix4f viewProjection =
		Math::Perspective(60.0f, 16.0f / 9.0f, 0.1f, 500.0f)
		* Math::LookAt(Vector3f(0.0f), Vector3f(0.0f, 0.0f, -1.0f), Vector3f(0.0f, 1.0f, 0.0f));

	std::vector<std::pair<EntityID, AABB>> boxes;
	boxes.reserve(kBoxes);
	for (int i = 0; i < kBoxes; ++i)
	{
		const Vector3f centre((float)(i % 100) - 50.0f, 0.0f, -(float)(i / 100) - 5.0f);
		boxes.emplace_back((EntityID)i, AABB{ centre - Vector3f(0.5f), centre + Vector3f(0.5f) });
	}

	std::vector<EntityID> visible;
	EB_BENCH_BUDGET("frustum test, 5000 AABBs", 3.0, 50, {
		visible.clear();
		GetEntitiesInFrustum(boxes, viewProjection, visible);
		});

	EB_NOTE("visible after cull: " + std::to_string(visible.size()) + " / " + std::to_string(kBoxes));
	EB_EXPECT_GT(visible.size(), (size_t)0);
	EB_EXPECT_LT(visible.size(), (size_t)kBoxes);
}
