// ECS TESTS
// ---------
// The hand-rolled ECS is the substrate every other system stands on, so a defect here is never
// local - it shows up as "physics forgot a body" or "an entity stopped rendering" three systems
// downstream. These tests drive Registry / ComponentManager / EntityManager / View directly
// (no Scene, no Application state) so a failure points at the ECS itself.
//
// The highest-value tests in this file are the sparse-set ones. ComponentMemoryArray removes with
// swap-and-pop, which means every removal MOVES an unrelated component into the freed slot and has
// to fix up that entity's sparse index. Get that wrong and components silently belong to the wrong
// entity - the kind of bug that is agony to find from a symptom.
//
// NOTE ON THE API: Registry::AttachComponent<T>(entity, args...) takes the entity FIRST, unlike
// Entity::AttachComponent<T>(args...) which infers it. Mixing them up compiles for components whose
// first constructor parameter is numeric (a float silently converts to EntityID), so keep them
// straight.

#include <Ember.h>

#include "TestFramework.h"
#include "TestHelpers.h"

#include <algorithm>
#include <string>
#include <vector>

using namespace Ember;
using Ember::Test::Type::Unit;
using Ember::Test::Type::Integration;

//////////////////////////////////////////////////////////////////////////
// Component type registration
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Ecs, ComponentTypeIdsAreDistinctAndInRange, Unit)
{
	Registry registry;

	// Every component type must fit inside EntityManager's std::bitset<MaxComponents> mask.
	// Adding a 65th component type (with MaxComponents == 64) would trip the bounds assert in
	// Debug and silently corrupt the mask in Release, so this is the guard rail for that.
	const std::vector<ComponentType> types = {
		registry.GetComponentType<IDComponent>(),
		registry.GetComponentType<TagComponent>(),
		registry.GetComponentType<RelationshipComponent>(),
		registry.GetComponentType<TransformComponent>(),
		registry.GetComponentType<DisabledComponent>(),
		registry.GetComponentType<RigidBodyComponent>(),
		registry.GetComponentType<BoxColliderComponent>(),
		registry.GetComponentType<SphereColliderComponent>(),
		registry.GetComponentType<CapsuleColliderComponent>(),
		registry.GetComponentType<ConvexMeshColliderComponent>(),
		registry.GetComponentType<ConcaveMeshColliderComponent>(),
		registry.GetComponentType<CharacterControllerComponent>(),
		registry.GetComponentType<SpriteComponent>(),
		registry.GetComponentType<StaticMeshComponent>(),
		registry.GetComponentType<SkinnedMeshComponent>(),
		registry.GetComponentType<MaterialComponent>(),
		registry.GetComponentType<CameraComponent>(),
		registry.GetComponentType<DirectionalLightComponent>(),
		registry.GetComponentType<SpotLightComponent>(),
		registry.GetComponentType<PointLightComponent>(),
		registry.GetComponentType<ScriptComponent>(),
		registry.GetComponentType<OutlineComponent>(),
		registry.GetComponentType<EditorIconComponent>(),
		registry.GetComponentType<AnimatorComponent>(),
		registry.GetComponentType<BoneSocketComponent>(),
		registry.GetComponentType<PrefabComponent>(),
		registry.GetComponentType<LifetimeComponent>(),
		registry.GetComponentType<TextComponent>(),
		registry.GetComponentType<PoolComponent>(),
		registry.GetComponentType<PoolConfigComponent>(),
		registry.GetComponentType<ParticleEmitterComponent>(),
		registry.GetComponentType<PostProcessVolumeComponent>(),
		registry.GetComponentType<AudioSourceComponent>(),
		registry.GetComponentType<SingleSoundComponent>(),
		registry.GetComponentType<AudioListenerComponent>(),
		registry.GetComponentType<WaypointComponent>(),
		registry.GetComponentType<AIAgentComponent>(),
		registry.GetComponentType<AIPathComponent>(),
		registry.GetComponentType<NavigationGridComponent>(),
		registry.GetComponentType<NavigationMeshComponent>(),
		registry.GetComponentType<NavigationMeshModifierComponent>(),
		registry.GetComponentType<LocalAvoidanceComponent>(),
		registry.GetComponentType<CanvasComponent>(),
		registry.GetComponentType<RectTransformComponent>(),
	};

	ComponentType highest = 0;
	for (ComponentType type : types)
	{
		EB_EXPECT_MSG(type < Constants::Entities::MaxComponents,
			"component type " + std::to_string(type) + " exceeds MaxComponents ("
			+ std::to_string(Constants::Entities::MaxComponents) + ") - raise MaxComponents in Constants.h");
		highest = Math::Max(highest, type);
	}

	// No two component types may share an ID, or their storage aliases.
	std::vector<ComponentType> sorted = types;
	std::sort(sorted.begin(), sorted.end());
	EB_EXPECT_MSG(std::adjacent_find(sorted.begin(), sorted.end()) == sorted.end(),
		"two component types were assigned the same id");

	EB_NOTE("highest component type id: " + std::to_string(highest)
		+ " / " + std::to_string(Constants::Entities::MaxComponents) + " slots");
}

EB_TEST_CASE(Ecs, ComponentTypeIdsAreStableAcrossRegistries, Unit)
{
	// Type IDs come from a process-wide counter (a function-local static inside
	// ComponentManager::GetComponentType<T>), so every registry in the process must agree on them.
	// Scenes hold one registry each while systems, prefab instantiation and Scene::CopyScene move
	// components between them; a per-registry numbering would make a component land in the wrong
	// array the moment two scenes are alive at once.
	//
	// (These IDs are deliberately NOT persisted - SceneSerializer writes component order by name -
	// so their absolute values are free to change between builds.)
	Registry first;
	Registry second;

	EB_EXPECT_EQ(first.GetComponentType<TransformComponent>(), second.GetComponentType<TransformComponent>());
	EB_EXPECT_EQ(first.GetComponentType<PointLightComponent>(), second.GetComponentType<PointLightComponent>());
	EB_EXPECT_NE(first.GetComponentType<TransformComponent>(), first.GetComponentType<PointLightComponent>());
}

//////////////////////////////////////////////////////////////////////////
// Attach / detach / contains
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Ecs, AttachDetachContains, Unit)
{
	Registry registry;
	const EntityID a = registry.CreateEntity();
	const EntityID b = registry.CreateEntity();

	EB_EXPECT_FALSE(registry.ContainsComponent<PointLightComponent>(a));

	auto& light = registry.AttachComponent<PointLightComponent>(a);
	EB_EXPECT(registry.ContainsComponent<PointLightComponent>(a));
	EB_EXPECT_FALSE(registry.ContainsComponent<PointLightComponent>(b));

	// Defaults come from the member initializers - proves real construction, not just storage.
	EB_EXPECT_NEAR(light.Intensity, 25.0f, 1e-5);
	EB_EXPECT_VEC3_NEAR(light.Color, Vector3f(1.0f), 1e-6f);

	registry.DetachComponent<PointLightComponent>(a);
	EB_EXPECT_FALSE(registry.ContainsComponent<PointLightComponent>(a));
}

EB_TEST_CASE(Ecs, AttachForwardsConstructorArguments, Unit)
{
	Registry registry;
	const EntityID entity = registry.CreateEntity();

	auto& light = registry.AttachComponent<PointLightComponent>(entity, Vector3f(1.0f, 0.5f, 0.25f), 12.0f, 3.0f);
	EB_EXPECT_VEC3_NEAR(light.Color, Vector3f(1.0f, 0.5f, 0.25f), 1e-6f);
	EB_EXPECT_NEAR(light.Intensity, 12.0f, 1e-5);
	EB_EXPECT_NEAR(light.Radius, 3.0f, 1e-5);
}

EB_TEST_CASE(Ecs, ReattachReplacesInPlace, Unit)
{
	// ComponentMemoryArray::AttachComponent assigns over an existing component rather than
	// pushing a duplicate. Scene::AddEntity relies on this (it attaches TransformComponent, then
	// callers re-attach with real values), so a duplicate here would leak storage every spawn.
	Registry registry;
	const EntityID entity = registry.CreateEntity();

	registry.AttachComponent<PointLightComponent>(entity, Vector3f(1.0f), 5.0f, 1.0f);
	registry.AttachComponent<PointLightComponent>(entity, Vector3f(0.0f, 1.0f, 0.0f), 50.0f, 2.0f);

	EB_EXPECT_EQ(registry.GetActiveEntities<PointLightComponent>().size(), (size_t)1);
	EB_EXPECT_NEAR(registry.GetComponent<PointLightComponent>(entity).Intensity, 50.0f, 1e-5);
	EB_EXPECT_VEC3_NEAR(registry.GetComponent<PointLightComponent>(entity).Color, Vector3f(0.0f, 1.0f, 0.0f), 1e-6f);
}

EB_TEST_CASE(Ecs, DetachByTypeIdMatchesTemplateDetach, Unit)
{
	Registry registry;
	const EntityID entity = registry.CreateEntity();
	registry.AttachComponent<LifetimeComponent>(entity, 2.0f);

	const ComponentType type = registry.GetComponentType<LifetimeComponent>();
	EB_CHECK(registry.ContainsComponent(entity, type));

	// The type-erased path is what the editor's "remove component" button uses.
	registry.DetachComponent(entity, type);
	EB_EXPECT_FALSE(registry.ContainsComponent(entity, type));
	EB_EXPECT_FALSE(registry.ContainsComponent<LifetimeComponent>(entity));
}

//////////////////////////////////////////////////////////////////////////
// Sparse-set integrity (swap-and-pop)
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Ecs, RemovalKeepsRemainingComponentsCorrectlyMapped, Unit)
{
	// Removal is swap-and-pop: the LAST component is moved into the removed slot and its owner's
	// sparse index is patched. Remove from the middle and every survivor must still resolve to its
	// own data. Tagging each component with a unique value makes a mis-mapping unmissable.
	Registry registry;
	constexpr int kCount = 32;

	std::vector<EntityID> entities;
	for (int i = 0; i < kCount; ++i)
	{
		const EntityID entity = registry.CreateEntity();
		entities.push_back(entity);
		registry.AttachComponent<LifetimeComponent>(entity, static_cast<float>(i));
	}

	EB_CHECK_EQ(registry.GetActiveEntities<LifetimeComponent>().size(), (size_t)kCount);

	// Remove every third entity, from the middle outward.
	std::vector<EntityID> removed;
	for (int i = 3; i < kCount; i += 3)
	{
		registry.DetachComponent<LifetimeComponent>(entities[i]);
		removed.push_back(entities[i]);
	}

	EB_EXPECT_EQ(registry.GetActiveEntities<LifetimeComponent>().size(), (size_t)(kCount - removed.size()));

	for (int i = 0; i < kCount; ++i)
	{
		const bool wasRemoved = std::find(removed.begin(), removed.end(), entities[i]) != removed.end();
		if (wasRemoved)
		{
			EB_EXPECT_MSG(!registry.ContainsComponent<LifetimeComponent>(entities[i]),
				"entity " + std::to_string(i) + " should no longer have a LifetimeComponent");
			continue;
		}

		EB_EXPECT_MSG(registry.ContainsComponent<LifetimeComponent>(entities[i]),
			"entity " + std::to_string(i) + " lost its component during an unrelated removal");

		// The payload must still be the value this entity was created with.
		EB_EXPECT_MSG(std::abs(registry.GetComponent<LifetimeComponent>(entities[i]).Lifetime - (float)i) < 1e-4f,
			"entity " + std::to_string(i) + " resolved to another entity's component data");
	}
}

EB_TEST_CASE(Ecs, DenseArrayStaysPackedAfterRemovals, Unit)
{
	// The dense array is what every View iterates. It must contain exactly the live owners,
	// with no duplicates and no stale entries.
	Registry registry;

	std::vector<EntityID> entities;
	for (int i = 0; i < 10; ++i)
	{
		const EntityID entity = registry.CreateEntity();
		entities.push_back(entity);
		registry.AttachComponent<LifetimeComponent>(entity, (float)i);
	}

	registry.DetachComponent<LifetimeComponent>(entities[0]); // first
	registry.DetachComponent<LifetimeComponent>(entities[9]); // last
	registry.DetachComponent<LifetimeComponent>(entities[4]); // middle

	std::vector<EntityID> dense = registry.GetActiveEntities<LifetimeComponent>();
	EB_CHECK_EQ(dense.size(), (size_t)7);

	std::sort(dense.begin(), dense.end());
	EB_EXPECT_MSG(std::adjacent_find(dense.begin(), dense.end()) == dense.end(),
		"dense entity array contains duplicates");

	for (EntityID id : dense)
	{
		EB_EXPECT_NE(id, entities[0]);
		EB_EXPECT_NE(id, entities[4]);
		EB_EXPECT_NE(id, entities[9]);
	}
}

EB_TEST_CASE(Ecs, DestroyEntityClearsEveryComponent, Unit)
{
	Registry registry;
	const EntityID entity = registry.CreateEntity();

	registry.AttachComponent<TransformComponent>(entity, Vector3f(1.0f, 2.0f, 3.0f), Vector3f(0.0f), Vector3f(1.0f));
	registry.AttachComponent<PointLightComponent>(entity);
	registry.AttachComponent<LifetimeComponent>(entity, 5.0f);

	EB_CHECK_EQ(registry.GetActiveEntities<PointLightComponent>().size(), (size_t)1);

	registry.DestroyEntity(entity);

	// Every per-type array must have dropped it - a leak in any one of them means the next
	// entity to reuse this ID inherits a ghost component.
	EB_EXPECT_EQ(registry.GetActiveEntities<TransformComponent>().size(), (size_t)0);
	EB_EXPECT_EQ(registry.GetActiveEntities<PointLightComponent>().size(), (size_t)0);
	EB_EXPECT_EQ(registry.GetActiveEntities<LifetimeComponent>().size(), (size_t)0);
}

EB_TEST_CASE(Ecs, RecycledEntityIdStartsClean, Unit)
{
	// EntityManager recycles destroyed IDs from a free list. A recycled ID must come back with an
	// empty component mask, otherwise a fresh entity inherits the dead one's components.
	Registry registry;

	const EntityID first = registry.CreateEntity();
	registry.AttachComponent<PointLightComponent>(first);
	registry.AttachComponent<LifetimeComponent>(first, 1.0f);
	registry.DestroyEntity(first);

	const EntityID recycled = registry.CreateEntity();
	EB_EXPECT_EQ(recycled, first); // FIFO free list hands the same ID back

	EB_EXPECT_FALSE(registry.ContainsComponent<PointLightComponent>(recycled));
	EB_EXPECT_FALSE(registry.ContainsComponent<LifetimeComponent>(recycled));
	EB_EXPECT_EQ(registry.GetComponentOrder(recycled).size(), (size_t)0);
}

//////////////////////////////////////////////////////////////////////////
// Component ordering (drives the editor inspector)
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Ecs, ComponentOrderTracksAttachAndDetach, Unit)
{
	Registry registry;
	const EntityID entity = registry.CreateEntity();

	registry.AttachComponent<TransformComponent>(entity);
	registry.AttachComponent<PointLightComponent>(entity);
	registry.AttachComponent<LifetimeComponent>(entity, 1.0f);

	EB_CHECK_EQ(registry.GetComponentOrder(entity).size(), (size_t)3);
	EB_EXPECT_EQ(registry.GetComponentOrder(entity)[0], registry.GetComponentType<TransformComponent>());
	EB_EXPECT_EQ(registry.GetComponentOrder(entity)[2], registry.GetComponentType<LifetimeComponent>());

	// Re-attaching an existing component must not append a second entry.
	registry.AttachComponent<PointLightComponent>(entity);
	EB_EXPECT_EQ(registry.GetComponentOrder(entity).size(), (size_t)3);

	registry.DetachComponent<PointLightComponent>(entity);
	EB_CHECK_EQ(registry.GetComponentOrder(entity).size(), (size_t)2);

	const auto& order = registry.GetComponentOrder(entity);
	const ComponentType lightType = registry.GetComponentType<PointLightComponent>();
	EB_EXPECT_MSG(std::find(order.begin(), order.end(), lightType) == order.end(),
		"detached component is still listed in the component order");
}

EB_TEST_CASE(Ecs, SetComponentOrderReordersAndIgnoresJunk, Unit)
{
	Registry registry;
	const EntityID entity = registry.CreateEntity();

	registry.AttachComponent<TransformComponent>(entity);
	registry.AttachComponent<PointLightComponent>(entity);
	registry.AttachComponent<LifetimeComponent>(entity, 1.0f);

	const ComponentType transformType = registry.GetComponentType<TransformComponent>();
	const ComponentType lightType = registry.GetComponentType<PointLightComponent>();
	const ComponentType lifetimeType = registry.GetComponentType<LifetimeComponent>();
	const ComponentType absentType = registry.GetComponentType<SpriteComponent>();

	// Requested order lists a component the entity does not have, plus a duplicate. Both must be
	// dropped, and any component the request omits must still survive (appended at the end).
	registry.SetComponentOrder(entity, { lifetimeType, absentType, lightType, lifetimeType });

	const auto& order = registry.GetComponentOrder(entity);
	EB_CHECK_EQ(order.size(), (size_t)3);
	EB_EXPECT_EQ(order[0], lifetimeType);
	EB_EXPECT_EQ(order[1], lightType);
	EB_EXPECT_EQ(order[2], transformType); // omitted from the request, appended afterwards
}

//////////////////////////////////////////////////////////////////////////
// Views and queries
//////////////////////////////////////////////////////////////////////////

namespace {

	// Views iterate EntityIDs; collecting them makes the assertions readable.
	template<typename ViewType>
	std::vector<EntityID> Collect(const ViewType& view)
	{
		std::vector<EntityID> out;
		for (EntityID entity : view)
			out.push_back(entity);
		return out;
	}

	bool Contains(const std::vector<EntityID>& entities, EntityID entity)
	{
		return std::find(entities.begin(), entities.end(), entity) != entities.end();
	}

} // namespace

EB_TEST_CASE(Ecs, QueryDriverOnly, Unit)
{
	Registry registry;
	const EntityID withLight = registry.CreateEntity();
	const EntityID withoutLight = registry.CreateEntity();

	registry.AttachComponent<PointLightComponent>(withLight);
	registry.AttachComponent<TransformComponent>(withoutLight);

	const std::vector<EntityID> found = Collect(registry.Query<PointLightComponent>());
	EB_EXPECT_EQ(found.size(), (size_t)1);
	EB_EXPECT(Contains(found, withLight));
	EB_EXPECT_FALSE(Contains(found, withoutLight));
}

EB_TEST_CASE(Ecs, QueryRequiresEveryFilterComponent, Unit)
{
	Registry registry;

	const EntityID both = registry.CreateEntity();
	registry.AttachComponent<PointLightComponent>(both);
	registry.AttachComponent<TransformComponent>(both);

	const EntityID driverOnly = registry.CreateEntity();
	registry.AttachComponent<PointLightComponent>(driverOnly);

	const EntityID filterOnly = registry.CreateEntity();
	registry.AttachComponent<TransformComponent>(filterOnly);

	const std::vector<EntityID> found = Collect(registry.Query<PointLightComponent, TransformComponent>());
	EB_CHECK_EQ(found.size(), (size_t)1);
	EB_EXPECT_EQ(found[0], both);
}

EB_TEST_CASE(Ecs, QueryWithExplicitExcludes, Unit)
{
	Registry registry;

	const EntityID plain = registry.CreateEntity();
	registry.AttachComponent<PointLightComponent>(plain);

	const EntityID excluded = registry.CreateEntity();
	registry.AttachComponent<PointLightComponent>(excluded);
	registry.AttachComponent<LifetimeComponent>(excluded, 1.0f);

	const std::vector<EntityID> found = Collect(registry.Query<PointLightComponent>(Exclude<LifetimeComponent>{}));
	EB_CHECK_EQ(found.size(), (size_t)1);
	EB_EXPECT_EQ(found[0], plain);
}

EB_TEST_CASE(Ecs, ActiveQuerySkipsDisabledEntities, Unit)
{
	// ActiveQuery is the gameplay-facing wrapper: it must hide anything carrying DisabledComponent.
	// Every runtime system uses it, so a break here quietly resurrects disabled entities.
	Registry registry;

	const EntityID enabled = registry.CreateEntity();
	registry.AttachComponent<PointLightComponent>(enabled);

	const EntityID disabled = registry.CreateEntity();
	registry.AttachComponent<PointLightComponent>(disabled);
	registry.AttachComponent<DisabledComponent>(disabled);

	const std::vector<EntityID> active = Collect(registry.ActiveQuery<PointLightComponent>());
	EB_CHECK_EQ(active.size(), (size_t)1);
	EB_EXPECT_EQ(active[0], enabled);

	// The unfiltered query still sees both.
	EB_EXPECT_EQ(Collect(registry.Query<PointLightComponent>()).size(), (size_t)2);

	// Re-enabling must bring it back.
	registry.DetachComponent<DisabledComponent>(disabled);
	EB_EXPECT_EQ(Collect(registry.ActiveQuery<PointLightComponent>()).size(), (size_t)2);
}

EB_TEST_CASE(Ecs, ActiveQueryMergesCustomExcludes, Unit)
{
	// ActiveQuery(Exclude<X>{}) must apply BOTH X and DisabledComponent, not replace one with
	// the other.
	Registry registry;

	const EntityID keep = registry.CreateEntity();
	registry.AttachComponent<PointLightComponent>(keep);

	const EntityID hasCustomExclude = registry.CreateEntity();
	registry.AttachComponent<PointLightComponent>(hasCustomExclude);
	registry.AttachComponent<LifetimeComponent>(hasCustomExclude, 1.0f);

	const EntityID isDisabled = registry.CreateEntity();
	registry.AttachComponent<PointLightComponent>(isDisabled);
	registry.AttachComponent<DisabledComponent>(isDisabled);

	const std::vector<EntityID> found = Collect(registry.ActiveQuery<PointLightComponent>(Exclude<LifetimeComponent>{}));
	EB_CHECK_EQ(found.size(), (size_t)1);
	EB_EXPECT_EQ(found[0], keep);
}

EB_TEST_CASE(Ecs, ViewFrontAndEmpty, Unit)
{
	Registry registry;

	auto emptyView = registry.Query<PointLightComponent>();
	EB_EXPECT(emptyView.Empty());
	EB_EXPECT_EQ(emptyView.Front(), (EntityID)Constants::Entities::InvalidEntityID);

	const EntityID entity = registry.CreateEntity();
	registry.AttachComponent<PointLightComponent>(entity);

	auto populatedView = registry.Query<PointLightComponent>();
	EB_EXPECT_FALSE(populatedView.Empty());
	EB_EXPECT_EQ(populatedView.Front(), entity);
}

EB_TEST_CASE(Ecs, QueryOnNeverRegisteredTypeIsEmptyNotACrash, Unit)
{
	// Querying a component type that no entity in this registry has ever used must return an empty
	// view rather than indexing a component array that was never allocated.
	Registry registry;
	registry.CreateEntity();

	auto view = registry.Query<PostProcessVolumeComponent>();
	EB_EXPECT(view.Empty());
	EB_EXPECT_EQ(Collect(view).size(), (size_t)0);
	EB_EXPECT_EQ(registry.GetActiveEntities<PostProcessVolumeComponent>().size(), (size_t)0);
}

EB_TEST_CASE(Ecs, GetComponentsReturnsLiveReferences, Unit)
{
	// GetComponents returns a tuple of REFERENCES; systems mutate through it every frame.
	// If it ever degraded to copies, every system would write into a temporary.
	Registry registry;
	const EntityID entity = registry.CreateEntity();
	registry.AttachComponent<TransformComponent>(entity, Vector3f(0.0f), Vector3f(0.0f), Vector3f(1.0f));
	registry.AttachComponent<LifetimeComponent>(entity, 3.0f);

	auto [transform, lifetime] = registry.GetComponents<TransformComponent, LifetimeComponent>(entity);
	transform.Position = Vector3f(9.0f, 8.0f, 7.0f);
	lifetime.Lifetime = 0.5f;

	EB_EXPECT_VEC3_NEAR(registry.GetComponent<TransformComponent>(entity).Position, Vector3f(9.0f, 8.0f, 7.0f), 1e-6f);
	EB_EXPECT_NEAR(registry.GetComponent<LifetimeComponent>(entity).Lifetime, 0.5f, 1e-6);
}

//////////////////////////////////////////////////////////////////////////
// Component lifecycle hooks
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Ecs, AttachCallbackFires, Unit)
{
	// PhysicsSystem uses these hooks to spawn rp3d bodies the instant a component appears.
	Registry registry;

	int callCount = 0;
	EntityID seenEntity = (EntityID)Constants::Entities::InvalidEntityID;
	registry.OnComponentAttached<PointLightComponent>().Connect(
		[&](EntityID entity, PointLightComponent& light) {
			++callCount;
			seenEntity = entity;
			light.Intensity = 123.0f; // the callback receives a live reference
		});

	const EntityID entity = registry.CreateEntity();
	registry.AttachComponent<PointLightComponent>(entity);

	EB_EXPECT_EQ(callCount, 1);
	EB_EXPECT_EQ(seenEntity, entity);
	EB_EXPECT_NEAR(registry.GetComponent<PointLightComponent>(entity).Intensity, 123.0f, 1e-5);
}

EB_TEST_CASE(Ecs, DetachCallbackFiresBeforeRemoval, Unit)
{
	// The detach hook must run while the component is STILL VALID - PhysicsSystem reads
	// Collider/Body pointers out of it to unregister them from the rp3d world.
	Registry registry;

	bool componentWasStillReadable = false;
	registry.OnComponentDetached<LifetimeComponent>().Connect(
		[&](EntityID, LifetimeComponent& lifetime) {
			componentWasStillReadable = std::abs(lifetime.Lifetime - 7.0f) < 1e-4f;
		});

	const EntityID entity = registry.CreateEntity();
	registry.AttachComponent<LifetimeComponent>(entity, 7.0f);
	registry.DetachComponent<LifetimeComponent>(entity);

	EB_EXPECT_MSG(componentWasStillReadable, "detach callback saw destroyed or default-constructed data");
	EB_EXPECT_FALSE(registry.ContainsComponent<LifetimeComponent>(entity));
}

EB_TEST_CASE(Ecs, TypeErasedDetachTriggersCallback, Unit)
{
	// Detaching by ComponentType (the editor's remove-component path) must fire the same hooks as
	// the templated overload, or removing a collider in the editor leaks its rp3d shape.
	Registry registry;

	int callCount = 0;
	registry.OnComponentDetached<LifetimeComponent>().Connect(
		[&](EntityID, LifetimeComponent&) { ++callCount; });

	const EntityID entity = registry.CreateEntity();
	registry.AttachComponent<LifetimeComponent>(entity, 1.0f);
	registry.DetachComponent(entity, registry.GetComponentType<LifetimeComponent>());

	EB_EXPECT_EQ(callCount, 1);
	EB_EXPECT_FALSE(registry.ContainsComponent<LifetimeComponent>(entity));
}

EB_TEST_CASE(Ecs, ConnectAndRetroactBackfillsExistingComponents, Unit)
{
	// This is how a system attaching to an already-populated scene catches up. If the backfill
	// misses entities, loading a saved scene produces objects with no physics body / GPU resource.
	Registry registry;

	std::vector<EntityID> preExisting;
	for (int i = 0; i < 5; ++i)
	{
		const EntityID entity = registry.CreateEntity();
		registry.AttachComponent<PointLightComponent>(entity);
		preExisting.push_back(entity);
	}

	std::vector<EntityID> visited;
	registry.ConnectAndRetroact<PointLightComponent>(
		[&](EntityID entity, PointLightComponent&) { visited.push_back(entity); });

	EB_CHECK_EQ(visited.size(), preExisting.size());
	for (EntityID entity : preExisting)
		EB_EXPECT(Contains(visited, entity));

	// ...and it stays connected for components added afterwards.
	const EntityID late = registry.CreateEntity();
	registry.AttachComponent<PointLightComponent>(late);
	EB_EXPECT_EQ(visited.size(), preExisting.size() + 1);
	EB_EXPECT(Contains(visited, late));
}

EB_TEST_CASE(Ecs, MultipleCallbacksAllFire, Unit)
{
	Registry registry;

	int first = 0, second = 0;
	registry.OnComponentAttached<PointLightComponent>().Connect([&](EntityID, PointLightComponent&) { ++first; });
	registry.OnComponentAttached<PointLightComponent>().Connect([&](EntityID, PointLightComponent&) { ++second; });

	const EntityID entity = registry.CreateEntity();
	registry.AttachComponent<PointLightComponent>(entity);

	EB_EXPECT_EQ(first, 1);
	EB_EXPECT_EQ(second, 1);
}

//////////////////////////////////////////////////////////////////////////
// Capacity
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Ecs, FillsToCapacityAndRecycles, Integration)
{
	// Exercises the entity pool right up to MaxEntities, then proves the whole set can be
	// destroyed and reallocated - the pattern a scene reload performs.
	Registry registry;
	const int capacity = (int)Constants::Entities::MaxEntities;

	std::vector<EntityID> entities;
	entities.reserve(capacity);
	for (int i = 0; i < capacity; ++i)
	{
		const EntityID entity = registry.CreateEntity();
		EB_EXPECT_MSG(entity < Constants::Entities::MaxEntities,
			"CreateEntity handed out an out-of-range id: " + std::to_string(entity));
		entities.push_back(entity);
		registry.AttachComponent<LifetimeComponent>(entity, (float)i);
	}

	EB_EXPECT_EQ(registry.GetActiveEntities<LifetimeComponent>().size(), (size_t)capacity);

	for (EntityID entity : entities)
		registry.DestroyEntity(entity);

	EB_EXPECT_EQ(registry.GetActiveEntities<LifetimeComponent>().size(), (size_t)0);

	// Every ID must be reusable - a leak in the free list would make the second fill assert.
	for (int i = 0; i < capacity; ++i)
	{
		const EntityID entity = registry.CreateEntity();
		EB_EXPECT_MSG(entity < Constants::Entities::MaxEntities,
			"recycled id out of range on refill: " + std::to_string(entity));
	}
}
