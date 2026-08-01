// SCENE / HIERARCHY TESTS
// -----------------------
// Scene owns entity identity (UUID <-> EntityID), the parent/child graph, ordering, duplication and
// deferred removal. Almost every authoring workflow in Ember-Forge routes through these calls, and
// most of them are graph surgery where a missed edge shows up much later as a dangling UUID.
//
// A note on how removal is tested: Scene::RemoveEntity() only QUEUES the entity. The queue drains in
// Scene::RemovePendingRemovals(), which is private and called only at the tail of a full frame - so
// the removal tests run SceneFixture::TickEdit() and are tagged Integration rather than Unit.

#include <Ember.h>

#include "TestFramework.h"
#include "TestHelpers.h"

#include <algorithm>
#include <string>
#include <vector>

using namespace Ember;
using Ember::Test::Type::Integration;
using Ember::Test::SceneFixture;
using Ember::Test::MakeEntityAt;

namespace {

	std::vector<UUID> EntityUUIDs(const std::vector<Entity>& entities)
	{
		std::vector<UUID> out;
		out.reserve(entities.size());
		for (Entity entity : entities)
			out.push_back(entity.GetUUID());
		return out;
	}

	bool ContainsUUID(const std::vector<UUID>& uuids, UUID uuid)
	{
		return std::find(uuids.begin(), uuids.end(), uuid) != uuids.end();
	}

	// Position of a UUID within a list, or -1. Used to assert sibling/root ordering.
	int IndexOfUUID(const std::vector<UUID>& uuids, UUID uuid)
	{
		for (size_t i = 0; i < uuids.size(); ++i)
		{
			if (uuids[i] == uuid)
				return (int)i;
		}
		return -1;
	}

} // namespace

//////////////////////////////////////////////////////////////////////////
// Entity creation and lookup
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Scene, AddEntityAttachesTheCoreComponents, Integration)
{
	SceneFixture scene("AddEntityScene");
	Entity entity = scene->AddEntity("Hero");

	// Every entity is expected by the rest of the engine to carry these four unconditionally.
	EB_EXPECT(entity.ContainsComponent<IDComponent>());
	EB_EXPECT(entity.ContainsComponent<TagComponent>());
	EB_EXPECT(entity.ContainsComponent<TransformComponent>());
	EB_EXPECT(entity.ContainsComponent<RelationshipComponent>());

	EB_EXPECT_EQ(entity.GetName(), std::string("Hero"));
	EB_EXPECT_NE(entity.GetUUID(), UUID(Constants::InvalidUUID));
	EB_EXPECT(entity.IsRootParent());
	EB_EXPECT(entity.IsActive());
	EB_EXPECT_EQ(entity.GetNumChildren(), (uint32_t)0);
}

EB_TEST_CASE(Scene, UnnamedEntityGetsADefaultName, Integration)
{
	SceneFixture scene;
	Entity entity = scene->AddEntity();
	EB_EXPECT_EQ(entity.GetName(), std::string("Entity"));
}

EB_TEST_CASE(Scene, LookupByUuidNameAndHandle, Integration)
{
	SceneFixture scene;
	Entity original = scene->AddEntity("Findable");
	const UUID uuid = original.GetUUID();

	Entity byUuid = scene->GetEntity(uuid);
	Entity byName = scene->GetEntity("Findable");
	Entity byHandle = scene->GetEntityByHandle(original.GetEntityHandle());

	EB_EXPECT(original == byUuid);
	EB_EXPECT(original == byName);
	EB_EXPECT(original == byHandle);

	std::string name;
	EB_EXPECT(scene->TryGetEntityName(uuid, name));
	EB_EXPECT_EQ(name, std::string("Findable"));
}

EB_TEST_CASE(Scene, LookupMissesReturnInvalidEntities, Integration)
{
	SceneFixture scene;
	scene->AddEntity("Present");

	// A miss must produce a default-constructed (invalid) Entity, not an out-of-range handle
	// that later indexes real component storage.
	EB_EXPECT_FALSE((scene->GetEntity(UUID(999999999))).IsValid());
	EB_EXPECT_FALSE((scene->GetEntity("NotHere")).IsValid());
	EB_EXPECT_FALSE((scene->GetEntityByHandle((EntityID)Constants::Entities::InvalidEntityID)).IsValid());

	std::string name;
	EB_EXPECT_FALSE(scene->TryGetEntityName(UUID(999999999), name));
}

EB_TEST_CASE(Scene, EveryEntityGetsAUniqueUuid, Integration)
{
	SceneFixture scene;
	std::vector<UUID> uuids;
	for (int i = 0; i < 200; ++i)
		uuids.push_back(scene->AddEntity().GetUUID());

	std::sort(uuids.begin(), uuids.end());
	EB_EXPECT_MSG(std::adjacent_find(uuids.begin(), uuids.end()) == uuids.end(),
		"two entities in the same scene were given the same UUID");
	EB_EXPECT_EQ(scene->GetAllEntities().size(), (size_t)200);
}

//////////////////////////////////////////////////////////////////////////
// Parent / child graph
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Scene, AddChildLinksBothDirections, Integration)
{
	SceneFixture scene;
	Entity parent = scene->AddEntity("Parent");
	Entity child = parent.AddChild("Child");

	// The link must exist from both ends - a one-sided edge is what produces orphaned entities
	// after a save/load cycle.
	EB_EXPECT_EQ(parent.GetNumChildren(), (uint32_t)1);
	EB_EXPECT(child.GetParent() == parent);
	EB_EXPECT_FALSE(child.IsRootParent());
	EB_EXPECT(parent.IsRootParent());
	EB_EXPECT(parent.GetChildByName("Child") == child);
}

EB_TEST_CASE(Scene, HierarchyTraversalIsRecursive, Integration)
{
	SceneFixture scene;
	Entity root = scene->AddEntity("Root");
	Entity childA = root.AddChild("ChildA");
	Entity childB = root.AddChild("ChildB");
	Entity grandchild = childA.AddChild("Grandchild");

	// GetAllChildren is a full descendant walk, not just direct children.
	const std::vector<UUID> descendants = EntityUUIDs(root.GetAllChildren());
	EB_EXPECT_EQ(descendants.size(), (size_t)3);
	EB_EXPECT(ContainsUUID(descendants, childA.GetUUID()));
	EB_EXPECT(ContainsUUID(descendants, childB.GetUUID()));
	EB_EXPECT(ContainsUUID(descendants, grandchild.GetUUID()));

	// GetChildByName only looks at DIRECT children; FindEntityInHierarchy recurses.
	EB_EXPECT_FALSE((root.GetChildByName("Grandchild")).IsValid());
	EB_EXPECT(root.FindEntityInHierarchy("Grandchild") == grandchild);
	EB_EXPECT_FALSE((root.FindEntityInHierarchy("NoSuchEntity")).IsValid());

	EB_EXPECT(grandchild.GetRootParent() == root);
}

EB_TEST_CASE(Scene, RemoveFromParentClearsBothSides, Integration)
{
	SceneFixture scene;
	Entity parent = scene->AddEntity("Parent");
	Entity child = parent.AddChild("Child");

	child.RemoveFromParent();

	EB_EXPECT(child.IsRootParent());
	EB_EXPECT_EQ(parent.GetNumChildren(), (uint32_t)0);
	EB_EXPECT_FALSE((child.GetParent()).IsValid());

	// The attachment flag must be cleared too, or re-parenting later silently inherits it.
	EB_EXPECT_FALSE(child.GetComponent<RelationshipComponent>().IsAttachment);
}

EB_TEST_CASE(Scene, SetEntityParentPreservesWorldPosition, Integration)
{
	// Re-parenting in the editor must not visually move the entity: the new local transform is
	// derived as inverse(parentWorld) * childWorld. This is the single most user-visible piece of
	// hierarchy maths in the engine.
	SceneFixture scene;

	Entity parent = MakeEntityAt(*scene, "Parent", Vector3f(10.0f, 0.0f, 0.0f));
	Entity child = MakeEntityAt(*scene, "Child", Vector3f(1.0f, 2.0f, 3.0f));

	// World transforms must be current before re-parenting - the maths reads them.
	scene.UpdateTransforms();
	const Vector3f worldBefore = child.GetComponent<TransformComponent>().GetWorldPosition();

	scene->SetEntityParent(child.GetUUID(), parent);
	scene.UpdateTransforms();

	const Vector3f worldAfter = child.GetComponent<TransformComponent>().GetWorldPosition();
	EB_EXPECT_VEC3_NEAR(worldAfter, worldBefore, 1e-3f);

	// The local offset is now relative to the parent.
	EB_EXPECT_VEC3_NEAR(child.GetComponent<TransformComponent>().Position, Vector3f(-9.0f, 2.0f, 3.0f), 1e-3f);
	EB_EXPECT(child.GetParent() == parent);
	EB_EXPECT_EQ(parent.GetNumChildren(), (uint32_t)1);
}

EB_TEST_CASE(Scene, SetEntityParentMovesBetweenParents, Integration)
{
	// Re-parenting from A to B must remove the child from A's list, not just add it to B's.
	// A leftover entry in A leaves a UUID pointing at an entity that no longer considers A its parent.
	SceneFixture scene;

	Entity first = MakeEntityAt(*scene, "First", Vector3f(0.0f));
	Entity second = MakeEntityAt(*scene, "Second", Vector3f(5.0f, 0.0f, 0.0f));
	Entity child = MakeEntityAt(*scene, "Child", Vector3f(1.0f, 0.0f, 0.0f));

	scene.UpdateTransforms();
	scene->SetEntityParent(child.GetUUID(), first);
	scene.UpdateTransforms();
	EB_CHECK_EQ(first.GetNumChildren(), (uint32_t)1);

	scene->SetEntityParent(child.GetUUID(), second);
	scene.UpdateTransforms();

	EB_EXPECT_EQ(first.GetNumChildren(), (uint32_t)0);
	EB_EXPECT_EQ(second.GetNumChildren(), (uint32_t)1);
	EB_EXPECT(child.GetParent() == second);
}

EB_TEST_CASE(Scene, SetEntityParentToSameParentIsANoOp, Integration)
{
	SceneFixture scene;
	Entity parent = scene->AddEntity("Parent");
	Entity child = scene->AddEntity("Child");

	scene.UpdateTransforms();
	scene->SetEntityParent(child.GetUUID(), parent);
	scene->SetEntityParent(child.GetUUID(), parent); // repeated

	// Re-issuing the same parenting must not append a duplicate child entry.
	EB_EXPECT_EQ(parent.GetNumChildren(), (uint32_t)1);
}

// REGRESSION GUARD: this test caught re-parenting failing to invalidate the child's world
// transform. Entity::AddChild only rewrites RelationshipComponent, and TransformSystem skips any
// node that is not dirty and whose parent did not change - so a freshly re-parented child kept the
// stale world transform it had as a root (the normal child stayed at x=1 instead of being pushed
// out to x=4 by the parent's scale). In the editor that showed up as "dragging an entity onto a
// parent leaves it in the wrong place". Fixed by TransformComponent::InvalidateWorld().
EB_TEST_CASE(Scene, AttachmentChildIgnoresParentScale, Integration)
{
	// Attachments (weapons on a bone socket, etc) deliberately inherit position and rotation but
	// NOT scale, so scaling a character doesn't stretch the sword.
	SceneFixture scene;

	Entity parent = MakeEntityAt(*scene, "Parent", Vector3f(0.0f), Vector3f(0.0f), Vector3f(4.0f));
	Entity attachment = MakeEntityAt(*scene, "Attachment", Vector3f(1.0f, 0.0f, 0.0f));
	Entity normalChild = MakeEntityAt(*scene, "NormalChild", Vector3f(1.0f, 0.0f, 0.0f));

	scene.UpdateTransforms();
	parent.AddChild(attachment, /*isAttachment*/ true);
	parent.AddChild(normalChild);
	scene.UpdateTransforms();

	// The regular child is pushed out to x=4 by the parent's scale...
	EB_EXPECT_NEAR(normalChild.GetComponent<TransformComponent>().GetWorldPosition().x, 4.0f, 1e-3);
	// ...the attachment stays at its unscaled offset.
	EB_EXPECT_NEAR(attachment.GetComponent<TransformComponent>().GetWorldPosition().x, 1.0f, 1e-3);
}

//////////////////////////////////////////////////////////////////////////
// Ordering
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Scene, GetAllEntitiesFollowsInsertionOrder, Integration)
{
	// The scene hierarchy panel renders in this order, so it must be stable and complete.
	SceneFixture scene;
	Entity first = scene->AddEntity("First");
	Entity second = scene->AddEntity("Second");
	Entity third = scene->AddEntity("Third");

	const std::vector<UUID> order = EntityUUIDs(scene->GetAllEntities());
	EB_CHECK_EQ(order.size(), (size_t)3);
	EB_EXPECT_EQ(order[0], first.GetUUID());
	EB_EXPECT_EQ(order[1], second.GetUUID());
	EB_EXPECT_EQ(order[2], third.GetUUID());
}

EB_TEST_CASE(Scene, ReorderEntityMovesRootSiblings, Integration)
{
	SceneFixture scene;
	Entity a = scene->AddEntity("A");
	Entity b = scene->AddEntity("B");
	Entity c = scene->AddEntity("C");
	scene.UpdateTransforms();

	// Move C before A.
	scene->ReorderEntity(c.GetUUID(), a.GetUUID(), /*insertAfter*/ false);

	std::vector<UUID> order = EntityUUIDs(scene->GetAllEntities());
	EB_EXPECT_LT(IndexOfUUID(order, c.GetUUID()), IndexOfUUID(order, a.GetUUID()));
	EB_EXPECT_EQ(order.size(), (size_t)3);

	// Move C after B - it should end up last.
	scene->ReorderEntity(c.GetUUID(), b.GetUUID(), /*insertAfter*/ true);
	order = EntityUUIDs(scene->GetAllEntities());
	EB_EXPECT_EQ(order[2], c.GetUUID());
}

EB_TEST_CASE(Scene, ReorderRejectsCycles, Integration)
{
	// Dropping a parent onto its own descendant would create a cycle, and the transform pass
	// recurses over the hierarchy - a cycle is an immediate stack overflow, not a soft failure.
	SceneFixture scene;
	Entity parent = scene->AddEntity("Parent");
	Entity child = parent.AddChild("Child");
	Entity grandchild = child.AddChild("Grandchild");
	scene.UpdateTransforms();

	scene->ReorderEntity(parent.GetUUID(), grandchild.GetUUID(), /*insertAfter*/ true);

	// The parent must NOT have been adopted into its own subtree.
	EB_EXPECT(parent.IsRootParent());
	EB_EXPECT(child.GetParent() == parent);
	EB_EXPECT(grandchild.GetParent() == child);
}

EB_TEST_CASE(Scene, MoveEntityToRootEndUnparentsAndAppends, Integration)
{
	SceneFixture scene;
	Entity parent = scene->AddEntity("Parent");
	Entity child = parent.AddChild("Child");
	scene->AddEntity("Other");
	scene.UpdateTransforms();

	scene->MoveEntityToRootEnd(child.GetUUID());

	EB_EXPECT(child.IsRootParent());
	EB_EXPECT_EQ(parent.GetNumChildren(), (uint32_t)0);

	const std::vector<UUID> order = EntityUUIDs(scene->GetAllEntities());
	EB_CHECK(!order.empty());
	EB_EXPECT_EQ(order.back(), child.GetUUID());
}

//////////////////////////////////////////////////////////////////////////
// Enable / disable
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Scene, SetActiveCascadesToDescendants, Integration)
{
	SceneFixture scene;
	Entity root = scene->AddEntity("Root");
	Entity child = root.AddChild("Child");
	Entity grandchild = child.AddChild("Grandchild");

	root.SetActive(false);
	EB_EXPECT_FALSE(root.IsActive());
	EB_EXPECT_FALSE(child.IsActive());
	EB_EXPECT_FALSE(grandchild.IsActive());

	root.SetActive(true);
	EB_EXPECT(root.IsActive());
	EB_EXPECT(child.IsActive());
	EB_EXPECT(grandchild.IsActive());
}

EB_TEST_CASE(Scene, SetActiveNonRecursiveLeavesChildren, Integration)
{
	SceneFixture scene;
	Entity root = scene->AddEntity("Root");
	Entity child = root.AddChild("Child");

	root.SetActive(false, /*recursive*/ false);
	EB_EXPECT_FALSE(root.IsActive());
	EB_EXPECT(child.IsActive());
}

EB_TEST_CASE(Scene, DisablingIsIdempotent, Integration)
{
	// SetActive(false) twice must not attach a second DisabledComponent, and re-enabling once
	// must fully clear it.
	SceneFixture scene;
	Entity entity = scene->AddEntity("Toggle");

	entity.SetActive(false);
	entity.SetActive(false);
	EB_EXPECT_FALSE(entity.IsActive());

	entity.SetActive(true);
	EB_EXPECT(entity.IsActive());
	EB_EXPECT_FALSE(entity.ContainsComponent<DisabledComponent>());
}

//////////////////////////////////////////////////////////////////////////
// Duplication
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Scene, DuplicateEntityCopiesDataWithNewIdentity, Integration)
{
	SceneFixture scene;
	Entity original = MakeEntityAt(*scene, "Light", Vector3f(1.0f, 2.0f, 3.0f));
	original.AttachComponent<PointLightComponent>(Vector3f(1.0f, 0.0f, 0.0f), 42.0f, 7.0f);
	scene.UpdateTransforms();

	Entity copy = scene->DuplicateEntity(original);
	EB_CHECK(copy.IsValid());

	// New identity...
	EB_EXPECT_NE(copy.GetUUID(), original.GetUUID());
	EB_EXPECT_NE(copy.GetName(), original.GetName());

	// ...same data.
	EB_CHECK(copy.ContainsComponent<PointLightComponent>());
	EB_EXPECT_NEAR(copy.GetComponent<PointLightComponent>().Intensity, 42.0f, 1e-4);
	EB_EXPECT_VEC3_NEAR(copy.GetComponent<PointLightComponent>().Color, Vector3f(1.0f, 0.0f, 0.0f), 1e-5f);
	EB_EXPECT_VEC3_NEAR(copy.GetComponent<TransformComponent>().Position, Vector3f(1.0f, 2.0f, 3.0f), 1e-5f);

	// Mutating the copy must not touch the original.
	copy.GetComponent<PointLightComponent>().Intensity = 1.0f;
	EB_EXPECT_NEAR(original.GetComponent<PointLightComponent>().Intensity, 42.0f, 1e-4);
}

// REGRESSION GUARD: this test caught a use-after-free in Scene::DuplicateEntityRecursive, which
// used to capture `auto& newRels = newEntity.AttachComponent<RelationshipComponent>()` and then
// write `newRels.Children.push_back(...)` AFTER recursing to duplicate each child. Every recursion
// calls AddEntity, which attaches another RelationshipComponent and can reallocate that component's
// dense vector - so the reference dangled and the child UUIDs landed in freed memory, silently
// losing the duplicated children. The fix collects them locally and re-fetches after the loop.
EB_TEST_CASE(Scene, DuplicateEntityCopiesChildren, Integration)
{
	SceneFixture scene;
	Entity root = MakeEntityAt(*scene, "Root", Vector3f(0.0f));
	Entity child = root.AddChild("Child");
	child.AttachComponent<PointLightComponent>();
	Entity grandchild = child.AddChild("Grandchild");
	scene.UpdateTransforms();

	const size_t before = scene->GetAllEntities().size();
	Entity copy = scene->DuplicateEntity(root);
	const size_t after = scene->GetAllEntities().size();

	// Root + child + grandchild == 3 new entities.
	EB_EXPECT_EQ(after - before, (size_t)3);
	EB_EXPECT_EQ(copy.GetAllChildren().size(), (size_t)2);

	// The duplicated subtree must point at the COPIES, not back at the originals.
	const std::vector<UUID> copyDescendants = EntityUUIDs(copy.GetAllChildren());
	EB_EXPECT_FALSE(ContainsUUID(copyDescendants, child.GetUUID()));
	EB_EXPECT_FALSE(ContainsUUID(copyDescendants, grandchild.GetUUID()));

	// The original subtree is untouched.
	EB_EXPECT_EQ(root.GetAllChildren().size(), (size_t)2);
}

EB_TEST_CASE(Scene, DuplicateKeepsTheOriginalsParent, Integration)
{
	// Duplicating a child must produce a sibling, not a new root.
	SceneFixture scene;
	Entity parent = scene->AddEntity("Parent");
	Entity child = parent.AddChild("Child");
	scene.UpdateTransforms();

	Entity copy = scene->DuplicateEntity(child);

	EB_EXPECT(copy.GetParent() == parent);
	EB_EXPECT_EQ(parent.GetNumChildren(), (uint32_t)2);
}

//////////////////////////////////////////////////////////////////////////
// Removal (deferred)
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Scene, RemoveEntityIsDeferredUntilEndOfFrame, Integration)
{
	// Removal must NOT take effect immediately - scripts routinely destroy entities from inside an
	// OnUpdate that is still iterating them, and pulling storage out mid-iteration invalidates the view.
	SceneFixture scene("RemovalScene");
	Entity keep = scene->AddEntity("Keep");
	Entity doomed = scene->AddEntity("Doomed");
	const UUID doomedUUID = doomed.GetUUID();

	scene->RemoveEntity(doomed);

	// Still present in the same frame.
	EB_EXPECT((scene->GetEntity(doomedUUID)).IsValid());
	EB_EXPECT_EQ(scene->GetAllEntities().size(), (size_t)2);

	scene.TickEdit();

	// Gone after the frame ends.
	EB_EXPECT_FALSE((scene->GetEntity(doomedUUID)).IsValid());
	EB_EXPECT_EQ(scene->GetAllEntities().size(), (size_t)1);
	EB_EXPECT((scene->GetEntity(keep.GetUUID())).IsValid());
}

EB_TEST_CASE(Scene, RemovingAParentRemovesItsSubtree, Integration)
{
	// Leaving orphans behind is the classic hierarchy leak: they keep rendering with a parent
	// UUID that no longer resolves.
	SceneFixture scene("SubtreeRemovalScene");
	Entity root = scene->AddEntity("Root");
	Entity child = root.AddChild("Child");
	Entity grandchild = child.AddChild("Grandchild");
	Entity bystander = scene->AddEntity("Bystander");

	const UUID rootUUID = root.GetUUID();
	const UUID childUUID = child.GetUUID();
	const UUID grandchildUUID = grandchild.GetUUID();

	scene->RemoveEntity(root);
	scene.TickEdit();

	EB_EXPECT_FALSE((scene->GetEntity(rootUUID)).IsValid());
	EB_EXPECT_FALSE((scene->GetEntity(childUUID)).IsValid());
	EB_EXPECT_FALSE((scene->GetEntity(grandchildUUID)).IsValid());

	EB_EXPECT((scene->GetEntity(bystander.GetUUID())).IsValid());
	EB_EXPECT_EQ(scene->GetAllEntities().size(), (size_t)1);
}

EB_TEST_CASE(Scene, RemovingAChildUnlinksItFromItsParent, Integration)
{
	SceneFixture scene("ChildRemovalScene");
	Entity parent = scene->AddEntity("Parent");
	Entity child = parent.AddChild("Child");

	scene->RemoveEntity(child);
	scene.TickEdit();

	// A stale UUID left in the parent's Children list would be dereferenced next frame.
	EB_EXPECT_EQ(parent.GetNumChildren(), (uint32_t)0);
	EB_EXPECT((scene->GetEntity(parent.GetUUID())).IsValid());
}

EB_TEST_CASE(Scene, ClearEmptiesTheScene, Integration)
{
	SceneFixture scene;
	for (int i = 0; i < 10; ++i)
		scene->AddEntity();

	EB_CHECK_EQ(scene->GetAllEntities().size(), (size_t)10);

	scene->Clear();
	EB_EXPECT_EQ(scene->GetAllEntities().size(), (size_t)0);

	// The scene must remain usable afterwards - Clear() rebuilds the registry rather than
	// leaving it in a torn-down state.
	Entity fresh = scene->AddEntity("AfterClear");
	EB_EXPECT(fresh.IsValid());
	EB_EXPECT_EQ(scene->GetAllEntities().size(), (size_t)1);
}

//////////////////////////////////////////////////////////////////////////
// Scene copying
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Scene, CopySceneKeepsUuidsAndRelationships, Integration)
{
	// Play mode copies the edit scene so stopping restores the authored state. UUIDs must be
	// PRESERVED (unlike DuplicateEntity) or every cross-entity reference in the copy breaks.
	SceneFixture source("SourceScene");
	Entity parent = MakeEntityAt(*source, "Parent", Vector3f(1.0f, 0.0f, 0.0f));
	Entity child = parent.AddChild("Child");
	child.AttachComponent<PointLightComponent>(Vector3f(0.0f, 1.0f, 0.0f), 9.0f, 2.0f);
	source.UpdateTransforms();

	SharedPtr<Scene> copy = Scene::CopyScene(source.Shared());
	EB_CHECK(copy != nullptr);

	EB_EXPECT_EQ(copy->GetAllEntities().size(), (size_t)2);

	Entity copiedParent = copy->GetEntity(parent.GetUUID());
	Entity copiedChild = copy->GetEntity(child.GetUUID());
	EB_CHECK_MSG(copiedParent.IsValid(), "parent UUID did not survive CopyScene");
	EB_CHECK_MSG(copiedChild.IsValid(), "child UUID did not survive CopyScene");

	// Relationships must be rebuilt in the copy.
	EB_EXPECT(copiedChild.GetParent() == copiedParent);
	EB_EXPECT_EQ(copiedParent.GetNumChildren(), (uint32_t)1);

	// Component data came across.
	EB_CHECK(copiedChild.ContainsComponent<PointLightComponent>());
	EB_EXPECT_NEAR(copiedChild.GetComponent<PointLightComponent>().Intensity, 9.0f, 1e-4);

	// The copy is independent: editing it must not write through to the source.
	copiedParent.GetComponent<TransformComponent>().Position = Vector3f(99.0f, 0.0f, 0.0f);
	EB_EXPECT_NEAR(parent.GetComponent<TransformComponent>().Position.x, 1.0f, 1e-4);
}
