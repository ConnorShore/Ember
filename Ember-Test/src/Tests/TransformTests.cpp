// TransformSystem turns local TRS into world matrices, and everything downstream reads WorldTransform.
// The dirty-flag tests matter because that flag is pure optimisation - when it breaks there is no
// error, just entities that silently stop moving.

#include <Ember.h>

#include "TestFramework.h"
#include "TestHelpers.h"

#include <string>

using namespace Ember;
using Ember::Test::Type::Integration;
using Ember::Test::SceneFixture;
using Ember::Test::MakeEntityAt;

//////////////////////////////////////////////////////////////////////////
// Basic propagation
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Transform, RootWorldMatchesLocal, Integration)
{
	SceneFixture scene;
	Entity entity = MakeEntityAt(*scene, "Mover", Vector3f(1.0f, 2.0f, 3.0f));

	scene.UpdateTransforms();

	auto& transform = entity.GetComponent<TransformComponent>();
	EB_EXPECT_VEC3_NEAR(transform.GetWorldPosition(), Vector3f(1.0f, 2.0f, 3.0f), 1e-4f);
	// A root has an identity parent, so its world matrix IS its local matrix.
	EB_EXPECT_MAT4_NEAR(transform.GetWorldTransform(), transform.GetLocalTransform(), 1e-5f);
}

EB_TEST_CASE(Transform, ChildTranslationIsRelativeToParent, Integration)
{
	SceneFixture scene;
	Entity parent = MakeEntityAt(*scene, "Parent", Vector3f(10.0f, 0.0f, 0.0f));
	Entity child = parent.AddChild("Child");
	child.GetComponent<TransformComponent>().Position = Vector3f(0.0f, 5.0f, 0.0f);

	scene.UpdateTransforms();

	EB_EXPECT_VEC3_NEAR(child.GetComponent<TransformComponent>().GetWorldPosition(), Vector3f(10.0f, 5.0f, 0.0f), 1e-4f);
}

EB_TEST_CASE(Transform, ChainOfThreeAccumulates, Integration)
{
	SceneFixture scene;
	Entity a = MakeEntityAt(*scene, "A", Vector3f(1.0f, 0.0f, 0.0f));
	Entity b = a.AddChild("B");
	Entity c = b.AddChild("C");

	b.GetComponent<TransformComponent>().Position = Vector3f(2.0f, 0.0f, 0.0f);
	c.GetComponent<TransformComponent>().Position = Vector3f(4.0f, 0.0f, 0.0f);

	scene.UpdateTransforms();

	EB_EXPECT_VEC3_NEAR(a.GetComponent<TransformComponent>().GetWorldPosition(), Vector3f(1.0f, 0.0f, 0.0f), 1e-4f);
	EB_EXPECT_VEC3_NEAR(b.GetComponent<TransformComponent>().GetWorldPosition(), Vector3f(3.0f, 0.0f, 0.0f), 1e-4f);
	EB_EXPECT_VEC3_NEAR(c.GetComponent<TransformComponent>().GetWorldPosition(), Vector3f(7.0f, 0.0f, 0.0f), 1e-4f);
}

EB_TEST_CASE(Transform, ParentRotationOrbitsChild, Integration)
{
	// A child offset along +X, with the parent yawed 90 degrees, must swing round to -Z.
	// Getting the multiplication order backwards produces +Z and nobody notices until a turret
	// shoots the wrong way.
	SceneFixture scene;
	Entity parent = MakeEntityAt(*scene, "Parent", Vector3f(0.0f), Vector3f(0.0f, Math::Radians(90.0f), 0.0f));
	Entity child = parent.AddChild("Child");
	child.GetComponent<TransformComponent>().Position = Vector3f(1.0f, 0.0f, 0.0f);

	scene.UpdateTransforms();

	EB_EXPECT_VEC3_NEAR(child.GetComponent<TransformComponent>().GetWorldPosition(), Vector3f(0.0f, 0.0f, -1.0f), 1e-4f);
}

EB_TEST_CASE(Transform, ParentScaleMultipliesChildOffsetAndScale, Integration)
{
	SceneFixture scene;
	Entity parent = MakeEntityAt(*scene, "Parent", Vector3f(0.0f), Vector3f(0.0f), Vector3f(3.0f));
	Entity child = parent.AddChild("Child");

	auto& childTransform = child.GetComponent<TransformComponent>();
	childTransform.Position = Vector3f(2.0f, 0.0f, 0.0f);
	childTransform.Scale = Vector3f(2.0f);

	scene.UpdateTransforms();

	// The offset is scaled by the parent...
	EB_EXPECT_VEC3_NEAR(childTransform.GetWorldPosition(), Vector3f(6.0f, 0.0f, 0.0f), 1e-4f);

	// ...and so is the child's own scale (3 * 2 == 6 along each axis).
	Vector3f worldT, worldR, worldS;
	EB_CHECK(Math::DecomposeTransform(childTransform.GetWorldTransform(), worldT, worldR, worldS));
	EB_EXPECT_VEC3_NEAR(worldS, Vector3f(6.0f), 1e-3f);
}

EB_TEST_CASE(Transform, MovingAParentMovesTheWholeSubtree, Integration)
{
	SceneFixture scene;
	Entity root = MakeEntityAt(*scene, "Root", Vector3f(0.0f));
	Entity child = root.AddChild("Child");
	Entity grandchild = child.AddChild("Grandchild");

	child.GetComponent<TransformComponent>().Position = Vector3f(1.0f, 0.0f, 0.0f);
	grandchild.GetComponent<TransformComponent>().Position = Vector3f(1.0f, 0.0f, 0.0f);
	scene.UpdateTransforms();

	EB_CHECK_VEC3_NEAR(grandchild.GetComponent<TransformComponent>().GetWorldPosition(), Vector3f(2.0f, 0.0f, 0.0f), 1e-4f);

	// Move only the root. Both descendants must follow on the very next pass - this is exactly what
	// the parentChanged propagation flag exists to guarantee.
	root.GetComponent<TransformComponent>().Position = Vector3f(0.0f, 10.0f, 0.0f);
	scene.UpdateTransforms();

	EB_EXPECT_VEC3_NEAR(child.GetComponent<TransformComponent>().GetWorldPosition(), Vector3f(1.0f, 10.0f, 0.0f), 1e-4f);
	EB_EXPECT_VEC3_NEAR(grandchild.GetComponent<TransformComponent>().GetWorldPosition(), Vector3f(2.0f, 10.0f, 0.0f), 1e-4f);
}

//////////////////////////////////////////////////////////////////////////
// Dirty tracking
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Transform, DirtyFlagClearsAfterUpdateAndSetsOnMutation, Integration)
{
	SceneFixture scene;
	Entity entity = MakeEntityAt(*scene, "Tracked", Vector3f(1.0f, 0.0f, 0.0f));

	auto& transform = entity.GetComponent<TransformComponent>();

	// Freshly constructed: no world transform has ever been built from this TRS.
	EB_EXPECT(transform.IsLocalDirty());

	scene.UpdateTransforms();
	EB_EXPECT_FALSE(transform.IsLocalDirty());

	// Each TRS field independently marks the transform dirty.
	transform.Position = Vector3f(2.0f, 0.0f, 0.0f);
	EB_EXPECT_MSG(transform.IsLocalDirty(), "changing Position did not mark the transform dirty");
	scene.UpdateTransforms();

	transform.Rotation = Vector3f(0.0f, 1.0f, 0.0f);
	EB_EXPECT_MSG(transform.IsLocalDirty(), "changing Rotation did not mark the transform dirty");
	scene.UpdateTransforms();

	transform.Scale = Vector3f(2.0f);
	EB_EXPECT_MSG(transform.IsLocalDirty(), "changing Scale did not mark the transform dirty");
	scene.UpdateTransforms();

	EB_EXPECT_FALSE(transform.IsLocalDirty());
}

EB_TEST_CASE(Transform, ReadingLocalTransformDoesNotClearTheWorldRebuildSignal, Integration)
{
	// The local-matrix cache and the world-rebuild snapshot are deliberately SEPARATE. If they
	// shared state, any caller that read GetLocalTransform() between the mutation and the transform
	// pass would clear the dirty flag and the entity would freeze in place for that frame.
	SceneFixture scene;
	Entity entity = MakeEntityAt(*scene, "Tracked", Vector3f(0.0f));
	auto& transform = entity.GetComponent<TransformComponent>();

	scene.UpdateTransforms();
	EB_CHECK_FALSE(transform.IsLocalDirty());

	transform.Position = Vector3f(5.0f, 0.0f, 0.0f);

	// Some unrelated system reads the local matrix (physics and the editor both do this).
	const Matrix4f local = transform.GetLocalTransform();
	EB_EXPECT_NEAR(local[3][0], 5.0f, 1e-5);

	// The world rebuild must still be pending.
	EB_EXPECT_MSG(transform.IsLocalDirty(), "GetLocalTransform() cleared the world-rebuild signal");

	scene.UpdateTransforms();
	EB_EXPECT_VEC3_NEAR(transform.GetWorldPosition(), Vector3f(5.0f, 0.0f, 0.0f), 1e-4f);
}

EB_TEST_CASE(Transform, LocalMatrixCacheRebuildsOnChange, Integration)
{
	SceneFixture scene;
	Entity entity = MakeEntityAt(*scene, "Cached", Vector3f(1.0f, 0.0f, 0.0f));
	auto& transform = entity.GetComponent<TransformComponent>();

	const Matrix4f first = transform.GetLocalTransform();
	// Repeat reads with no mutation must be identical (the cache is being used, not rebuilt wrongly).
	EB_EXPECT_MAT4_NEAR(transform.GetLocalTransform(), first, 1e-6f);

	transform.Position = Vector3f(9.0f, 0.0f, 0.0f);
	const Matrix4f second = transform.GetLocalTransform();
	EB_EXPECT_NEAR(second[3][0], 9.0f, 1e-5);
}

EB_TEST_CASE(Transform, DisabledRootIsSkippedByTheTransformPass, Integration)
{
	// The pass drives off ActiveQuery, so a disabled ROOT (and therefore its whole subtree) is
	// skipped entirely. Note the asymmetry: UpdateTransformTree recurses into children without
	// re-checking DisabledComponent, so a disabled CHILD of an ACTIVE root still gets updated.
	SceneFixture scene;
	Entity root = MakeEntityAt(*scene, "DisabledRoot", Vector3f(1.0f, 0.0f, 0.0f));
	Entity child = root.AddChild("Child");
	scene.UpdateTransforms();

	root.SetActive(false);
	root.GetComponent<TransformComponent>().Position = Vector3f(100.0f, 0.0f, 0.0f);
	child.GetComponent<TransformComponent>().Position = Vector3f(50.0f, 0.0f, 0.0f);
	scene.UpdateTransforms();

	// Neither the disabled root nor anything under it was rebuilt.
	EB_EXPECT_NEAR(root.GetComponent<TransformComponent>().GetWorldPosition().x, 1.0f, 1e-4);
	EB_EXPECT_NEAR(child.GetComponent<TransformComponent>().GetWorldPosition().x, 1.0f, 1e-4);

	// Re-enabling brings the whole subtree back into the pass.
	root.SetActive(true);
	scene.UpdateTransforms();
	EB_EXPECT_NEAR(root.GetComponent<TransformComponent>().GetWorldPosition().x, 100.0f, 1e-4);
	EB_EXPECT_NEAR(child.GetComponent<TransformComponent>().GetWorldPosition().x, 150.0f, 1e-4);
}

//////////////////////////////////////////////////////////////////////////
// Basis vectors
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Transform, BasisVectorsFromWorldMatrix, Integration)
{
	SceneFixture scene;
	Entity entity = MakeEntityAt(*scene, "Oriented", Vector3f(0.0f));
	scene.UpdateTransforms();

	auto& transform = entity.GetComponent<TransformComponent>();

	// With no rotation: forward is -Z, right is +X, up is +Y.
	EB_EXPECT_VEC3_NEAR(transform.GetForward(), Vector3f(0.0f, 0.0f, -1.0f), 1e-5f);
	EB_EXPECT_VEC3_NEAR(transform.GetRight(), Vector3f(1.0f, 0.0f, 0.0f), 1e-5f);
	EB_EXPECT_VEC3_NEAR(transform.GetUp(), Vector3f(0.0f, 1.0f, 0.0f), 1e-5f);

	// Yaw 90 degrees: forward swings to -X, right to -Z.
	transform.Rotation = Vector3f(0.0f, Math::Radians(90.0f), 0.0f);
	scene.UpdateTransforms();

	EB_EXPECT_VEC3_NEAR(transform.GetForward(), Vector3f(-1.0f, 0.0f, 0.0f), 1e-4f);
	EB_EXPECT_VEC3_NEAR(transform.GetRight(), Vector3f(0.0f, 0.0f, -1.0f), 1e-4f);
	EB_EXPECT_VEC3_NEAR(transform.GetUp(), Vector3f(0.0f, 1.0f, 0.0f), 1e-4f);
}

EB_TEST_CASE(Transform, BasisVectorsStayNormalisedUnderScale, Integration)
{
	// GetForward/Right/Up normalise, so a scaled entity still reports unit-length directions.
	// Skipping the normalise would make movement speed scale with the model, which is a
	// spectacularly confusing gameplay bug.
	SceneFixture scene;
	Entity entity = MakeEntityAt(*scene, "Scaled", Vector3f(0.0f), Vector3f(0.0f), Vector3f(7.0f, 3.0f, 5.0f));
	scene.UpdateTransforms();

	auto& transform = entity.GetComponent<TransformComponent>();
	EB_EXPECT_NEAR(Math::Length(transform.GetForward()), 1.0f, 1e-4);
	EB_EXPECT_NEAR(Math::Length(transform.GetRight()), 1.0f, 1e-4);
	EB_EXPECT_NEAR(Math::Length(transform.GetUp()), 1.0f, 1e-4);
}

//////////////////////////////////////////////////////////////////////////
// Stability
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Transform, RepeatedPassesDoNotDrift, Integration)
{
	// The pass must be a fixed point on a static hierarchy. If it ever accumulated (world = world *
	// local) idle objects would slowly drift away.
	SceneFixture scene;
	Entity root = MakeEntityAt(*scene, "Root", Vector3f(1.0f, 2.0f, 3.0f), Vector3f(0.0f, Math::Radians(35.0f), 0.0f), Vector3f(2.0f));
	Entity child = root.AddChild("Child");
	child.GetComponent<TransformComponent>().Position = Vector3f(1.0f, 1.0f, 1.0f);

	scene.UpdateTransforms();
	const Matrix4f rootAfterFirst = root.GetComponent<TransformComponent>().GetWorldTransform();
	const Matrix4f childAfterFirst = child.GetComponent<TransformComponent>().GetWorldTransform();

	for (int i = 0; i < 200; ++i)
		scene.UpdateTransforms();

	EB_EXPECT_MAT4_NEAR(root.GetComponent<TransformComponent>().GetWorldTransform(), rootAfterFirst, 1e-5f);
	EB_EXPECT_MAT4_NEAR(child.GetComponent<TransformComponent>().GetWorldTransform(), childAfterFirst, 1e-5f);
}

EB_TEST_CASE(Transform, WideAndDeepHierarchiesResolve, Integration)
{
	// A 64-deep chain plus a wide fan-out, to shake out recursion and ordering assumptions in
	// UpdateTransformTree. Each link adds exactly 1 on X, so the expected world position at any
	// depth is just the depth.
	SceneFixture scene;

	constexpr int kDepth = 64;
	Entity current = MakeEntityAt(*scene, "Depth0", Vector3f(0.0f));
	for (int i = 1; i < kDepth; ++i)
	{
		current = current.AddChild("Depth" + std::to_string(i));
		current.GetComponent<TransformComponent>().Position = Vector3f(1.0f, 0.0f, 0.0f);
	}

	Entity fanRoot = MakeEntityAt(*scene, "FanRoot", Vector3f(0.0f, 10.0f, 0.0f));
	for (int i = 0; i < 32; ++i)
	{
		Entity leaf = fanRoot.AddChild("Leaf" + std::to_string(i));
		leaf.GetComponent<TransformComponent>().Position = Vector3f((float)i, 0.0f, 0.0f);
	}

	scene.UpdateTransforms();

	EB_EXPECT_NEAR(current.GetComponent<TransformComponent>().GetWorldPosition().x, (float)(kDepth - 1), 1e-3);

	Entity leaf7 = fanRoot.GetChildByName("Leaf7");
	EB_CHECK(leaf7.IsValid());
	EB_EXPECT_VEC3_NEAR(leaf7.GetComponent<TransformComponent>().GetWorldPosition(), Vector3f(7.0f, 10.0f, 0.0f), 1e-3f);
}
