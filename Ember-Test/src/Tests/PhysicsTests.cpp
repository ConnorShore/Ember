// These target the seam between ECS components and ReactPhysics3D rather than rp3d's solver: bodies
// are created by lifecycle hooks, and components hold raw pointers into a world that is destroyed and
// rebuilt on every scene attach. Build the scene first, then Attach() - see TestHelpers.h.

#include <Ember.h>

#include "TestFramework.h"
#include "TestHelpers.h"

#include <string>

using namespace Ember;
using Ember::Test::Type::Integration;
using Ember::Test::PhysicsSceneFixture;
using Ember::Test::MakeEntityAt;
using Ember::Test::MakeStaticBox;
using Ember::Test::MakeDynamicBox;
using Ember::Test::Sys;

namespace {

	// Ground plane whose TOP SURFACE sits exactly at y == 0, so a unit cube resting on it settles
	// with its centre at y == 0.5. Keeps every expectation in the file easy to reason about.
	Entity MakeGround(Scene& scene)
	{
		return MakeStaticBox(scene, "Ground", Vector3f(0.0f, -0.5f, 0.0f), Vector3f(40.0f, 1.0f, 40.0f));
	}

	float WorldY(Entity entity)
	{
		return entity.GetComponent<TransformComponent>().GetWorldPosition().y;
	}

	// THE WORLD ORIGIN IS NOT EMPTY: PhysicsSystem parks a 0.1-radius camera-sensor trigger there in
	// every physics world, and it answers raycasts and overlap tests. Queries run in this offset lane
	// instead, so results contain only the bodies the test created.
	constexpr float kLaneX = 12.0f;
	constexpr float kLaneZ = 7.0f;

	Vector3f Lane(float y = 0.0f)
	{
		return Vector3f(kLaneX, y, kLaneZ);
	}

	const Vector3f kDown(0.0f, -1.0f, 0.0f);

} // namespace

//////////////////////////////////////////////////////////////////////////
// Body creation
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Physics, AttachCreatesBodiesAndColliders, Integration)
{
	PhysicsSceneFixture scene;
	Entity box = MakeDynamicBox(*scene, "Box", Vector3f(0.0f, 5.0f, 0.0f));
	scene.Attach();

	auto& rigidBody = box.GetComponent<RigidBodyComponent>();
	EB_CHECK_MSG(rigidBody.Body != nullptr, "no rp3d body was created for the RigidBodyComponent");

	auto& collider = box.GetComponent<BoxColliderComponent>();
	EB_EXPECT_MSG(collider.Shape != nullptr, "no rp3d shape was created for the BoxColliderComponent");
	EB_EXPECT_MSG(collider.Collider != nullptr, "the collider was never attached to a body");
	EB_EXPECT_MSG(collider.AttachedBody == rigidBody.Body, "collider is attached to the wrong body");
}

EB_TEST_CASE(Physics, BodySpawnsAtTheEntitysWorldPosition, Integration)
{
	// Bodies are built from WorldTransform. If a caller ever attaches physics before running the
	// transform pass, every body silently spawns at the origin - this is the canary for that.
	PhysicsSceneFixture scene;
	Entity box = MakeDynamicBox(*scene, "Box", Vector3f(3.0f, 7.0f, -2.0f));
	scene.Attach();

	auto& rigidBody = box.GetComponent<RigidBodyComponent>();
	EB_CHECK(rigidBody.Body != nullptr);

	const reactphysics3d::Vector3& position = rigidBody.Body->getTransform().getPosition();
	EB_EXPECT_NEAR(position.x, 3.0f, 1e-3);
	EB_EXPECT_NEAR(position.y, 7.0f, 1e-3);
	EB_EXPECT_NEAR(position.z, -2.0f, 1e-3);
}

EB_TEST_CASE(Physics, ChildColliderAttachesToTheAncestorBody, Integration)
{
	// A collider on a child entity is expected to find the rigid body on an ancestor and attach
	// there, at the correct relative offset. This is how compound colliders are authored.
	PhysicsSceneFixture scene;

	Entity parent = MakeEntityAt(*scene, "Parent", Vector3f(0.0f, 5.0f, 0.0f));
	parent.AttachComponent<RigidBodyComponent>(RigidBodyComponent::BodyType::Dynamic, 1.0f, true);

	Entity child = parent.AddChild("ChildCollider");
	child.GetComponent<TransformComponent>().Position = Vector3f(2.0f, 0.0f, 0.0f);
	child.AttachComponent<BoxColliderComponent>(Vector3f(1.0f));

	scene.Attach();

	auto& rigidBody = parent.GetComponent<RigidBodyComponent>();
	auto& collider = child.GetComponent<BoxColliderComponent>();

	EB_CHECK(rigidBody.Body != nullptr);
	EB_CHECK_MSG(collider.Collider != nullptr, "child collider was not created");
	EB_EXPECT_MSG(collider.AttachedBody == rigidBody.Body,
		"child collider did not attach to the ancestor's rigid body");
	EB_EXPECT_EQ(rigidBody.Body->getNbColliders(), (uint32_t)1);
}

//////////////////////////////////////////////////////////////////////////
// Simulation
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Physics, GravityPullsDynamicBodiesDown, Integration)
{
	PhysicsSceneFixture scene;
	Entity box = MakeDynamicBox(*scene, "Falling", Vector3f(0.0f, 20.0f, 0.0f));
	scene.Attach();

	const float startY = WorldY(box);
	scene.Step(60); // one second

	const float endY = WorldY(box);
	EB_NOTE("fell from y=" + std::to_string(startY) + " to y=" + std::to_string(endY) + " in 1s");

	// Free fall for 1s is ~4.9 m; keep the bounds wide enough to survive integrator differences
	// but tight enough to catch "gravity is off" or "gravity is 100x".
	EB_EXPECT_LT(endY, startY - 3.0f);
	EB_EXPECT_GT(endY, startY - 8.0f);

	// The simulated position must have been written back onto the entity transform, not left
	// only inside rp3d.
	EB_EXPECT_NEAR(box.GetComponent<TransformComponent>().Position.y, endY, 1e-3);
}

EB_TEST_CASE(Physics, StaticBodiesNeverMove, Integration)
{
	PhysicsSceneFixture scene;
	Entity ground = MakeGround(*scene);
	scene.Attach();

	const float startY = WorldY(ground);
	scene.Step(60);

	EB_EXPECT_NEAR(WorldY(ground), startY, 1e-5);
}

EB_TEST_CASE(Physics, DisablingGravityLeavesABodyFloating, Integration)
{
	PhysicsSceneFixture scene;
	Entity box = MakeEntityAt(*scene, "Floater", Vector3f(0.0f, 10.0f, 0.0f));
	box.AttachComponent<RigidBodyComponent>(RigidBodyComponent::BodyType::Dynamic, 1.0f, /*gravityEnabled*/ false);
	box.AttachComponent<BoxColliderComponent>(Vector3f(1.0f));
	scene.Attach();

	scene.Step(60);

	EB_EXPECT_NEAR(WorldY(box), 10.0f, 1e-3);
}

EB_TEST_CASE(Physics, DynamicBodyComesToRestOnGround, Integration)
{
	// Exercises the full contact path: broadphase, narrowphase, contact solving, and the
	// rp3d -> transform write-back. A unit cube on ground whose top is y == 0 settles at y == 0.5.
	PhysicsSceneFixture scene;
	MakeGround(*scene);
	Entity box = MakeDynamicBox(*scene, "Box", Vector3f(0.0f, 6.0f, 0.0f));
	scene.Attach();

	scene.Step(240); // four seconds - plenty to fall and settle

	const float restY = WorldY(box);
	EB_NOTE("resting height: " + std::to_string(restY) + " (expected ~0.5)");

	// Generous tolerance: solvers allow a little penetration at rest.
	EB_EXPECT_NEAR(restY, 0.5f, 0.25);

	// It must have actually STOPPED, not still be sinking.
	const float beforeExtraSteps = restY;
	scene.Step(60);
	EB_EXPECT_NEAR(WorldY(box), beforeExtraSteps, 0.05);

	// ...and it must not have tunnelled through the floor.
	EB_EXPECT_GT(WorldY(box), 0.0f);
}

EB_TEST_CASE(Physics, KinematicBodiesAreDrivenByTheEntityTransform, Integration)
{
	// Dynamic: physics writes the transform. Kinematic: the transform writes physics. Getting this
	// backwards makes scripted platforms and doors immovable.
	PhysicsSceneFixture scene;
	Entity platform = MakeEntityAt(*scene, "Platform", Vector3f(0.0f, 1.0f, 0.0f));
	platform.AttachComponent<RigidBodyComponent>(RigidBodyComponent::BodyType::Kinematic, 1.0f, true);
	platform.AttachComponent<BoxColliderComponent>(Vector3f(4.0f, 1.0f, 4.0f));
	scene.Attach();

	// Gravity must NOT move a kinematic body.
	scene.Step(30);
	EB_EXPECT_NEAR(WorldY(platform), 1.0f, 1e-4);

	// Moving the entity must move the rp3d body with it.
	platform.GetComponent<TransformComponent>().Position = Vector3f(0.0f, 5.0f, 0.0f);
	scene.Step(2);

	auto& rigidBody = platform.GetComponent<RigidBodyComponent>();
	EB_CHECK(rigidBody.Body != nullptr);
	EB_EXPECT_NEAR(rigidBody.Body->getTransform().getPosition().y, 5.0f, 1e-2);
	EB_EXPECT_NEAR(WorldY(platform), 5.0f, 1e-3);
}

EB_TEST_CASE(Physics, ImpulseChangesVelocityByImpulseOverMass, Integration)
{
	PhysicsSceneFixture scene;
	Entity box = MakeEntityAt(*scene, "Projectile", Vector3f(0.0f, 10.0f, 0.0f));
	box.AttachComponent<RigidBodyComponent>(RigidBodyComponent::BodyType::Dynamic, 2.0f, /*gravityEnabled*/ false);
	box.AttachComponent<BoxColliderComponent>(Vector3f(1.0f));
	scene.Attach();

	auto& rigidBody = box.GetComponent<RigidBodyComponent>();
	EB_CHECK(rigidBody.Body != nullptr);

	// The collider setup pass pushes RigidBodyComponent::Mass onto the rp3d body, so dv = J / m
	// is predictable: a 10 N.s impulse on a 2 kg body is 5 m/s.
	EB_EXPECT_NEAR(rigidBody.Body->getMass(), 2.0f, 1e-3);

	rigidBody.ApplyImpulse(Vector3f(10.0f, 0.0f, 0.0f));
	EB_EXPECT_NEAR(rigidBody.GetCurrentVelocity().x, 5.0f, 1e-3);

	const float startX = box.GetComponent<TransformComponent>().GetWorldPosition().x;
	scene.Step(60);
	EB_EXPECT_GT(box.GetComponent<TransformComponent>().GetWorldPosition().x, startX + 1.0f);
}

EB_TEST_CASE(Physics, DisabledEntityStopsSimulating, Integration)
{
	// Attaching DisabledComponent deactivates the rp3d body through a lifecycle hook. Without it,
	// "disabled" objects keep falling and keep generating contacts.
	PhysicsSceneFixture scene;
	Entity box = MakeDynamicBox(*scene, "Toggle", Vector3f(0.0f, 20.0f, 0.0f));
	scene.Attach();

	scene.Step(10);
	box.SetActive(false);
	const float frozenY = WorldY(box);

	scene.Step(60);
	EB_EXPECT_NEAR(WorldY(box), frozenY, 1e-3);

	// Re-enabling resumes the fall.
	box.SetActive(true);
	scene.Step(30);
	EB_EXPECT_LT(WorldY(box), frozenY);
}

EB_TEST_CASE(Physics, SimulationIsRepeatable, Integration)
{
	// Two identical scenes, stepped identically, must land in the same place. A single free-falling
	// body has no solver ordering to depend on, so any divergence points at leaked state between
	// runs (a stale time accumulator, a world that was not really restarted).
	auto runOnce = [](float& outY)
		{
			PhysicsSceneFixture scene;
			Entity box = MakeDynamicBox(*scene, "Falling", Vector3f(0.0f, 30.0f, 0.0f));
			scene.Attach();
			scene.Step(120);
			outY = WorldY(box);
		};

	float first = 0.0f, second = 0.0f;
	runOnce(first);
	runOnce(second);

	EB_NOTE("run A y=" + std::to_string(first) + "  run B y=" + std::to_string(second));
	EB_EXPECT_NEAR(first, second, 1e-3);
}

//////////////////////////////////////////////////////////////////////////
// Queries
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Physics, RaycastHitsAColliderAndReportsTheEntity, Integration)
{
	PhysicsSceneFixture scene;
	Entity ground = MakeGround(*scene);
	scene.Attach();

	const RaycastData hit = Sys<PhysicsSystem>()->CastRay(Lane(5.0f), kDown, 10.0f, FilterPreset::All);

	EB_CHECK_MSG(hit.Hit, "downward ray from above the ground did not hit it");
	EB_EXPECT_NEAR(hit.CollisionPoint.y, 0.0f, 1e-2);          // ground's top surface
	EB_EXPECT_VEC3_NEAR(hit.SurfaceNormal, Vector3f(0.0f, 1.0f, 0.0f), 1e-2f);
	EB_EXPECT_EQ(hit.RigidBodyEntity, ground.GetEntityHandle());
	EB_EXPECT_EQ(hit.ColliderEntity, ground.GetEntityHandle());

	// The ray starts 5 above a surface 5 units away over a 10-unit ray, so it hits at the midpoint.
	EB_EXPECT_NEAR(hit.HitFraction, 0.5f, 0.05);
}

EB_TEST_CASE(Physics, RaycastMissesEmptySpace, Integration)
{
	PhysicsSceneFixture scene;
	MakeGround(*scene);
	scene.Attach();

	// Well outside the 40x40 ground slab.
	const RaycastData miss = Sys<PhysicsSystem>()->CastRay(
		Vector3f(500.0f, 5.0f, 500.0f), Vector3f(0.0f, -1.0f, 0.0f), 10.0f, FilterPreset::All);
	EB_EXPECT_FALSE(miss.Hit);

	// Correct place, wrong direction.
	const RaycastData upward = Sys<PhysicsSystem>()->CastRay(
		Lane(5.0f), Vector3f(0.0f, 1.0f, 0.0f), 10.0f, FilterPreset::All);
	EB_EXPECT_FALSE(upward.Hit);
}

EB_TEST_CASE(Physics, RaycastReturnsTheNearestHit, Integration)
{
	// RaycastCallback returns hitFraction to shrink the ray, which is what makes rp3d keep only the
	// CLOSEST hit. Without it you get whichever collider the broadphase happened to visit first.
	PhysicsSceneFixture scene;
	MakeGround(*scene);
	Entity nearer = MakeStaticBox(*scene, "Nearer", Lane(3.0f), Vector3f(2.0f, 1.0f, 2.0f));
	scene.Attach();

	const RaycastData hit = Sys<PhysicsSystem>()->CastRay(Lane(10.0f), kDown, 20.0f, FilterPreset::All);

	EB_CHECK(hit.Hit);
	EB_EXPECT_MSG(hit.RigidBodyEntity == nearer.GetEntityHandle(),
		"raycast returned a farther collider instead of the nearest one");
	EB_EXPECT_NEAR(hit.CollisionPoint.y, 3.5f, 1e-2); // top of the raised box
}

EB_TEST_CASE(Physics, RaycastRespectsCategoryFilters, Integration)
{
	// Collision filtering is what keeps a "bullet" ray from hitting triggers, water volumes, or the
	// shooter. Both halves matter: the right filter must hit AND the wrong filter must miss.
	constexpr Filter kGroundCategory = (Filter)(1 << 3);
	constexpr Filter kOtherCategory = (Filter)(1 << 4);

	PhysicsSceneFixture scene;
	Entity ground = MakeGround(*scene);
	ground.GetComponent<BoxColliderComponent>().Category = kGroundCategory;
	scene.Attach();

	auto physics = Sys<PhysicsSystem>();
	const Vector3f origin = Lane(5.0f);

	EB_EXPECT_MSG(physics->CastRay(origin, kDown, 10.0f, FilterPreset::All).Hit,
		"ray with FilterPreset::All should hit everything");
	EB_EXPECT_MSG(physics->CastRay(origin, kDown, 10.0f, kGroundCategory).Hit,
		"ray filtered to the ground's own category should hit it");
	EB_EXPECT_MSG(!physics->CastRay(origin, kDown, 10.0f, kOtherCategory).Hit,
		"ray filtered to a different category should NOT hit the ground");
}

EB_TEST_CASE(Physics, OverlapSphereFindsOnlyNearbyBodies, Integration)
{
	// NOTE: do not name locals `near` / `far` here - they are macros from the Windows headers.
	// Both boxes sit in the query lane, away from the engine's camera sensor at the origin.
	PhysicsSceneFixture scene;
	Entity nearBox = MakeStaticBox(*scene, "NearBox", Lane(), Vector3f(1.0f));
	Entity farBox = MakeStaticBox(*scene, "FarBox", Vector3f(kLaneX + 50.0f, 0.0f, kLaneZ), Vector3f(1.0f));
	scene.Attach();

	const OverlapTestData overlaps = Sys<PhysicsSystem>()->TestOverlapSphere(
		Lane(), 2.0f, Entity(), FilterPreset::All);

	EB_CHECK_MSG(static_cast<bool>(overlaps), "sphere overlapping a collider reported no hits");
	EB_EXPECT_EQ(overlaps.Hits.size(), (size_t)1);
	EB_EXPECT_EQ(overlaps.Hits[0].EntityID, nearBox.GetEntityHandle());

	// Nothing is within a radius-2 probe 100 units above everything.
	const OverlapTestData empty = Sys<PhysicsSystem>()->TestOverlapSphere(
		Lane(100.0f), 2.0f, Entity(), FilterPreset::All);
	EB_EXPECT_FALSE(static_cast<bool>(empty));

	// A probe placed on the distant box finds it and only it.
	const OverlapTestData farOverlap = Sys<PhysicsSystem>()->TestOverlapSphere(
		Vector3f(kLaneX + 50.0f, 0.0f, kLaneZ), 2.0f, Entity(), FilterPreset::All);
	EB_CHECK(static_cast<bool>(farOverlap));
	EB_EXPECT_EQ(farOverlap.Hits.size(), (size_t)1);
	EB_EXPECT_EQ(farOverlap.Hits[0].EntityID, farBox.GetEntityHandle());
}

EB_TEST_CASE(Physics, OverlapBoxFindsColliders, Integration)
{
	PhysicsSceneFixture scene;
	Entity target = MakeStaticBox(*scene, "Target", Lane(), Vector3f(1.0f));
	scene.Attach();

	const OverlapTestData overlaps = Sys<PhysicsSystem>()->TestOverlapBox(
		Lane(), Vector3f(0.0f), Vector3f(2.0f), Entity(), FilterPreset::All);

	EB_CHECK(static_cast<bool>(overlaps));
	EB_EXPECT_EQ(overlaps.Hits.size(), (size_t)1);
	EB_EXPECT_EQ(overlaps.Hits[0].EntityID, target.GetEntityHandle());
}

//////////////////////////////////////////////////////////////////////////
// Lifetime of runtime physics state
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Physics, DetachingAColliderClearsItsRuntimePointers, Integration)
{
	// The detach hook must unregister the collider from the body AND null the component's raw
	// pointers. A missed null leaves a dangling rp3d pointer that is read next frame.
	PhysicsSceneFixture scene;
	Entity box = MakeDynamicBox(*scene, "Box", Vector3f(0.0f, 5.0f, 0.0f));
	scene.Attach();

	auto& rigidBody = box.GetComponent<RigidBodyComponent>();
	EB_CHECK(rigidBody.Body != nullptr);
	EB_CHECK_EQ(rigidBody.Body->getNbColliders(), (uint32_t)1);

	box.DetachComponent<BoxColliderComponent>();

	EB_EXPECT_EQ(rigidBody.Body->getNbColliders(), (uint32_t)0);
	EB_EXPECT_FALSE(box.ContainsComponent<BoxColliderComponent>());
}

EB_TEST_CASE(Physics, DetachingARigidBodyClearsColliderBackReferences, Integration)
{
	// Removing the body must also clear AttachedBody/Collider on every collider component that
	// pointed at it - otherwise the collider component outlives the body it references.
	PhysicsSceneFixture scene;
	Entity box = MakeDynamicBox(*scene, "Box", Vector3f(0.0f, 5.0f, 0.0f));
	scene.Attach();

	EB_CHECK(box.GetComponent<BoxColliderComponent>().AttachedBody != nullptr);

	box.DetachComponent<RigidBodyComponent>();

	auto& collider = box.GetComponent<BoxColliderComponent>();
	EB_EXPECT_MSG(collider.AttachedBody == nullptr, "collider still points at a destroyed rigid body");
	EB_EXPECT_MSG(collider.Collider == nullptr, "collider handle was not cleared when its body was destroyed");
}

EB_TEST_CASE(Physics, DuplicatedEntityGetsIndependentPhysicsObjects, Integration)
{
	// Duplication copies components verbatim, which would otherwise hand the copy the ORIGINAL's
	// rp3d pointers - two entities driving one body, and a double-free when either is destroyed.
	// Scene::DuplicateEntityRecursive resets the runtime state and re-initialises; this locks that in.
	PhysicsSceneFixture scene;
	Entity original = MakeDynamicBox(*scene, "Original", Vector3f(0.0f, 5.0f, 0.0f));
	scene.Attach();

	auto* originalBody = original.GetComponent<RigidBodyComponent>().Body;
	EB_CHECK(originalBody != nullptr);

	Entity copy = scene->DuplicateEntity(original);
	EB_CHECK(copy.IsValid());
	EB_CHECK(copy.ContainsComponent<RigidBodyComponent>());

	auto* copyBody = copy.GetComponent<RigidBodyComponent>().Body;
	EB_CHECK_MSG(copyBody != nullptr, "the duplicate never got its own rp3d body");
	EB_EXPECT_MSG(copyBody != originalBody, "the duplicate shares the original's rp3d body");

	auto& originalCollider = original.GetComponent<BoxColliderComponent>();
	auto& copyCollider = copy.GetComponent<BoxColliderComponent>();
	EB_EXPECT_MSG(copyCollider.Shape != originalCollider.Shape, "the duplicate shares the original's collision shape");
	EB_EXPECT_MSG(copyCollider.Collider != originalCollider.Collider, "the duplicate shares the original's collider");
}

EB_TEST_CASE(Physics, RemovingAnEntityDestroysItsBody, Integration)
{
	// Removal runs through the deferred queue, so this needs a full frame (TickEdit).
	// Afterwards the collider must be gone from the physics world, not merely from the ECS.
	PhysicsSceneFixture scene("PhysicsRemovalScene");
	MakeGround(*scene);
	Entity obstacle = MakeStaticBox(*scene, "Obstacle", Lane(3.0f), Vector3f(2.0f, 1.0f, 2.0f));
	scene.Attach();

	auto physics = Sys<PhysicsSystem>();
	const Vector3f origin = Lane(10.0f);

	const RaycastData before = physics->CastRay(origin, kDown, 20.0f, FilterPreset::All);
	EB_CHECK(before.Hit);
	EB_CHECK_EQ(before.RigidBodyEntity, obstacle.GetEntityHandle());

	const UUID obstacleUUID = obstacle.GetUUID();
	scene->RemoveEntity(obstacle);
	scene.TickEdit();

	EB_EXPECT_FALSE((scene->GetEntity(obstacleUUID)).IsValid());

	// The ray now falls through to the ground - proof the body really left the physics world.
	const RaycastData after = physics->CastRay(origin, kDown, 20.0f, FilterPreset::All);
	EB_EXPECT(after.Hit);
	EB_EXPECT_MSG(after.RigidBodyEntity != obstacle.GetEntityHandle(),
		"raycast still hits a collider whose entity was removed - the rp3d body leaked");
	EB_EXPECT_NEAR(after.CollisionPoint.y, 0.0f, 1e-2);
}

EB_TEST_CASE(Physics, ReattachingASceneRebuildsEveryBody, Integration)
{
	// OnSceneAttach destroys the rp3d world and rebuilds it from the components. Every body pointer
	// must be refreshed; a body left pointing into the destroyed world is a use-after-free the next
	// time the scene ticks. This is the exact path Play -> Stop -> Play takes.
	PhysicsSceneFixture scene;
	Entity box = MakeDynamicBox(*scene, "Box", Vector3f(0.0f, 5.0f, 0.0f));
	scene.Attach();

	auto* firstBody = box.GetComponent<RigidBodyComponent>().Body;
	EB_CHECK(firstBody != nullptr);

	scene.Attach(); // simulate re-entering play mode

	auto* secondBody = box.GetComponent<RigidBodyComponent>().Body;
	EB_CHECK_MSG(secondBody != nullptr, "re-attaching the scene left the entity without a body");

	// Deliberately not asserting secondBody != firstBody: the old world is destroyed first, so rp3d's
	// allocator often reuses the address. What matters is that the rebuilt world still simulates.
	EB_NOTE(std::string("body pointer ") + (secondBody == firstBody ? "was reused by the allocator" : "changed"));

	// And the rebuilt world still simulates correctly.
	const float startY = WorldY(box);
	scene.Step(60);
	EB_EXPECT_LT(WorldY(box), startY - 3.0f);
}
