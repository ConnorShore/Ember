// AssetManager lookup-table consistency (UUID/name/path) plus scene, prefab and asset round-trips.
// GetAsset asserts on a miss and RemoveAsset deletes the file - see the traps section of README.md.

#include <Ember.h>

#include "TestFramework.h"
#include "TestHelpers.h"

#include "Ember/Asset/Serializers/PhysicsMaterialSerializer.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <type_traits>
#include <vector>

using namespace Ember;
using Ember::Test::Type::Integration;
using Ember::Test::Type::Unit;
using Ember::Test::Assets;
using Ember::Test::MakeEntityAt;
using Ember::Test::SceneFixture;
using Ember::Test::TempFile;
using Ember::Test::TryGetAsset;

//////////////////////////////////////////////////////////////////////////
// Default engine assets
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Assets, DefaultEngineAssetsAreLoaded, Integration)
{
	// AssetManager::LoadDefaults() runs during Application construction. If these are missing the
	// working directory is wrong, and practically every other integration/visual test will fail in
	// a confusing way - so check it explicitly and once.
	struct Expected { uint64_t Uuid; const char* What; };
	const std::vector<Expected> expected = {
		{ Constants::Assets::DefaultWhiteTexUUID,        "DefaultWhite texture" },
		{ Constants::Assets::DefaultNormalTexUUID,       "DefaultNormal texture" },
		{ Constants::Assets::DefaultBlackTexUUID,        "DefaultBlack texture" },
		{ Constants::Assets::CubeMeshUUID,               "Cube primitive mesh" },
		{ Constants::Assets::SphereMeshUUID,             "Sphere primitive mesh" },
		{ Constants::Assets::QuadMeshUUID,               "Quad primitive mesh" },
		{ Constants::Assets::StandardGeometryMatUUID,    "StandardGeometry material" },
		{ Constants::Assets::StandardGeometryShadUUID,   "StandardGeometry shader" },
	};

	for (const Expected& entry : expected)
	{
		EB_EXPECT_MSG(Assets().ContainsAsset(UUID(entry.Uuid)),
			std::string("missing default asset: ") + entry.What);
	}

	// Spot-check that one of them actually resolves to the right concrete type, not just that the
	// UUID is present in the map.
	auto cube = TryGetAsset<Mesh>(UUID(Constants::Assets::CubeMeshUUID));
	EB_CHECK_MSG(cube != nullptr, "CubeMeshUUID is registered but does not resolve as a Mesh");
	EB_EXPECT_EQ(cube->GetType(), AssetType::Mesh);
	EB_EXPECT_GT(cube->GetTriangleCount(), (uint32_t)0);
}

EB_TEST_CASE(Assets, PrimitiveMeshBoundsAreSane, Integration)
{
	Ember::Test::RequireDefaultAssets();

	auto cube = TryGetAsset<Mesh>(UUID(Constants::Assets::CubeMeshUUID));
	EB_CHECK(cube != nullptr);

	const Vector3f minBounds = cube->GetMinBounds();
	const Vector3f maxBounds = cube->GetMaxBounds();

	// Bounds feed frustum culling and the physics mesh colliders. An inverted or zero-volume box
	// silently culls the object out of every view.
	EB_EXPECT_LT(minBounds.x, maxBounds.x);
	EB_EXPECT_LT(minBounds.y, maxBounds.y);
	EB_EXPECT_LT(minBounds.z, maxBounds.z);
	EB_NOTE("cube bounds: " + Ember::Test::ToString(minBounds) + " .. " + Ember::Test::ToString(maxBounds));
}

//////////////////////////////////////////////////////////////////////////
// AssetManager bookkeeping
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Assets, CreateRegistersInEveryLookupTable, Integration)
{
	const std::string name = "EmberTest_CreatedPhysMat";
	auto material = Assets().Create<PhysicsMaterial>(UUID(), name);
	EB_CHECK(material != nullptr);

	material->Friction = 0.75f;
	material->Bounciness = 0.25f;

	EB_EXPECT(Assets().ContainsAsset(material->GetUUID()));
	EB_EXPECT(Assets().ContainsAssetWithName(name));

	auto byUuid = TryGetAsset<PhysicsMaterial>(material->GetUUID());
	auto byName = TryGetAsset<PhysicsMaterial>(name);
	EB_CHECK(byUuid != nullptr);
	EB_CHECK(byName != nullptr);

	// Both lookups must land on the SAME object, not on two copies.
	EB_EXPECT_EQ(byUuid.Ptr(), byName.Ptr());
	EB_EXPECT_NEAR(byUuid->Friction, 0.75f, 1e-5);

	Assets().RemoveAsset(material->GetUUID());
	EB_EXPECT_FALSE(Assets().ContainsAsset(material->GetUUID()));
	EB_EXPECT_FALSE(Assets().ContainsAssetWithName(name));
}

EB_TEST_CASE(Assets, RenameUpdatesLookupsAndRejectsCollisions, Integration)
{
	const std::string originalName = "EmberTest_RenameSource";
	const std::string newName = "EmberTest_RenameTarget";
	const std::string occupiedName = "EmberTest_RenameOccupied";

	// ABSOLUTE paths on purpose: Register() stores the path verbatim while every lookup normalises
	// through std::filesystem::absolute(), so a relative path is invisible to the collision check.
	const std::string originalPath = std::filesystem::absolute(TempFile("rename_source.physmat")).string();
	const std::string newPath = std::filesystem::absolute(TempFile("rename_target.physmat")).string();
	const std::string occupiedPath = std::filesystem::absolute(TempFile("rename_occupied.physmat")).string();

	auto subject = Assets().Create<PhysicsMaterial>(UUID(), originalName, originalPath);
	auto blocker = Assets().Create<PhysicsMaterial>(UUID(), occupiedName, occupiedPath);
	EB_CHECK(subject != nullptr);
	EB_CHECK(blocker != nullptr);

	// Create() only indexes UUID and name; register the path explicitly so the path table is populated.
	Assets().Register<PhysicsMaterial>(subject);
	Assets().Register<PhysicsMaterial>(blocker);

	EB_CHECK(Assets().RenameAsset(subject->GetUUID(), newName, newPath));
	EB_EXPECT_EQ(subject->GetName(), newName);
	EB_EXPECT(Assets().ContainsAssetWithName(newName));
	EB_EXPECT_MSG(!Assets().ContainsAssetWithName(originalName),
		"the old name is still resolvable after a rename - the lookup table was not cleaned up");

	// Renaming onto a name another asset already owns must be refused rather than silently
	// stealing the mapping and orphaning the other asset.
	EB_EXPECT_FALSE(Assets().RenameAsset(subject->GetUUID(), occupiedName,
		std::filesystem::absolute(TempFile("rename_other.physmat")).string()));
	EB_EXPECT_EQ(subject->GetName(), newName); // unchanged

	// ...and so must renaming onto an occupied path.
	EB_EXPECT_FALSE(Assets().RenameAsset(subject->GetUUID(), "EmberTest_RenameThird", occupiedPath));

	Assets().RemoveAsset(subject->GetUUID());
	Assets().RemoveAsset(blocker->GetUUID());
}

EB_TEST_CASE(Assets, GetAssetsOfTypeFiltersByType, Integration)
{
	auto first = Assets().Create<PhysicsMaterial>(UUID(), "EmberTest_TypeFilterA");
	auto second = Assets().Create<PhysicsMaterial>(UUID(), "EmberTest_TypeFilterB");
	EB_CHECK(first != nullptr);
	EB_CHECK(second != nullptr);

	const std::vector<SharedPtr<PhysicsMaterial>> materials = Assets().GetAssetsOfType<PhysicsMaterial>();

	bool sawFirst = false, sawSecond = false;
	for (const auto& material : materials)
	{
		EB_EXPECT_EQ(material->GetType(), AssetType::PhysicsMaterial);
		if (material->GetUUID() == first->GetUUID())  sawFirst = true;
		if (material->GetUUID() == second->GetUUID()) sawSecond = true;
	}

	EB_EXPECT(sawFirst);
	EB_EXPECT(sawSecond);

	Assets().RemoveAsset(first->GetUUID());
	Assets().RemoveAsset(second->GetUUID());
}

//////////////////////////////////////////////////////////////////////////
// Asset file round-trips
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Assets, PhysicsMaterialSourceRoundTrip, Integration)
{
	const std::string path = TempFile("roundtrip.physmat");
	Ember::Test::RemoveTempFile(path);

	{
		auto material = SharedPtr<PhysicsMaterial>::Create(UUID(), "EmberTest_RoundTripMat", path);
		material->Friction = 0.375f;
		material->Bounciness = 0.625f;

		// NOTE: called directly rather than through AssetManager::SaveAssetToFile<PhysicsMaterial>().
		// That template branch passes (asset, path) while every serializer declares (path, asset), so
		// it does not compile - it is simply never instantiated today. See the README.
		EB_CHECK_MSG(PhysicsMaterialSerializer::Serialize(path, material), "failed to write the physics material");
	}

	EB_CHECK_MSG(std::filesystem::exists(path), "serializer reported success but wrote no file");

	auto loaded = PhysicsMaterialSerializer::Deserialize(UUID(), path);
	EB_CHECK_MSG(loaded != nullptr, "failed to read back the physics material just written");

	EB_EXPECT_NEAR(loaded->Friction, 0.375f, 1e-5);
	EB_EXPECT_NEAR(loaded->Bounciness, 0.625f, 1e-5);
	EB_EXPECT_EQ(loaded->GetName(), std::string("EmberTest_RoundTripMat"));

	Ember::Test::RemoveTempFile(path);
}

EB_TEST_CASE(Assets, PhysicsMaterialCookedRoundTrip, Integration)
{
	// The cooked (.bin) tier is what shipped builds actually read. It has its own reader/writer pair,
	// so a source-only test would not cover it at all - and a cooked-path regression only ever shows
	// up in a packaged build, which is the worst possible place to find one.
	const std::string sourcePath = TempFile("cooked_roundtrip.physmat");
	const std::string cookedPath = TempFile("cooked_roundtrip.bin");
	Ember::Test::RemoveTempFile(cookedPath);

	auto material = SharedPtr<PhysicsMaterial>::Create(UUID(), "EmberTest_CookedMat", sourcePath);
	material->Friction = 0.125f;
	material->Bounciness = 0.875f;

	EB_CHECK_MSG(PhysicsMaterialSerializer::SerializeCooked(cookedPath, material), "failed to write the cooked material");
	EB_CHECK(std::filesystem::exists(cookedPath));

	auto loaded = PhysicsMaterialSerializer::DeserializeCooked(UUID(), cookedPath);
	EB_CHECK_MSG(loaded != nullptr, "failed to read back the cooked material");
	EB_EXPECT_NEAR(loaded->Friction, 0.125f, 1e-5);
	EB_EXPECT_NEAR(loaded->Bounciness, 0.875f, 1e-5);

	Ember::Test::RemoveTempFile(cookedPath);
}

EB_TEST_CASE(Assets, LoadDeduplicatesByAbsolutePath, Integration)
{
	// Loading the same file twice must hand back the SAME asset. Without this, a mesh referenced by
	// twenty entities is decoded and uploaded twenty times.
	const std::string path = TempFile("dedupe.physmat");
	Ember::Test::RemoveTempFile(path);

	auto material = SharedPtr<PhysicsMaterial>::Create(UUID(), "EmberTest_DedupeMat", path);
	material->Friction = 0.5f;
	EB_CHECK(PhysicsMaterialSerializer::Serialize(path, material));

	auto first = Assets().Load<PhysicsMaterial>(path);
	EB_CHECK_MSG(first != nullptr, "first load failed");

	auto second = Assets().Load<PhysicsMaterial>(path);
	EB_CHECK_MSG(second != nullptr, "second load failed");

	EB_EXPECT_MSG(first.Ptr() == second.Ptr(), "loading the same path twice produced two separate assets");
	EB_EXPECT_EQ(first->GetUUID(), second->GetUUID());

	// A relative and an absolute spelling of the same file must also collapse to one asset.
	auto viaAbsolute = Assets().Load<PhysicsMaterial>(std::filesystem::absolute(path).string());
	EB_EXPECT_MSG(viaAbsolute.Ptr() == first.Ptr(), "absolute and relative paths produced separate assets");

	Assets().RemoveAsset(first->GetUUID());
	Ember::Test::RemoveTempFile(path);
}

//////////////////////////////////////////////////////////////////////////
// Scene serialization
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Serialization, SceneRoundTripPreservesComponentData, Integration)
{
	Ember::Test::RequireDefaultAssets();

	const std::string path = TempFile("scene_roundtrip.ebs");
	Ember::Test::RemoveTempFile(path);

	UUID savedUUID;
	{
		SceneFixture source("RoundTripScene");
		Entity entity = MakeEntityAt(*source, "Persisted",
			Vector3f(7.0f, -3.0f, 1.5f),
			Vector3f(0.0f, Math::Radians(45.0f), 0.0f),
			Vector3f(2.0f, 0.5f, 1.0f));
		savedUUID = entity.GetUUID();

		entity.AttachComponent<PointLightComponent>(Vector3f(0.25f, 0.5f, 0.75f), 33.0f, 4.5f);
		entity.AttachComponent<StaticMeshComponent>(UUID(Constants::Assets::CubeMeshUUID));
		entity.AttachComponent<MaterialComponent>(UUID(Constants::Assets::StandardGeometryMatUUID));
		entity.AttachComponent<RigidBodyComponent>(RigidBodyComponent::BodyType::Kinematic, 12.5f, false);
		entity.AttachComponent<LifetimeComponent>(9.5f);
		entity.AttachComponent<TextComponent>("Hello Ember", Vector4f(1.0f, 0.0f, 0.0f, 0.5f), UUID(Constants::InvalidUUID));

		source.UpdateTransforms();

		SceneSerializer serializer(source.Shared());
		EB_CHECK_MSG(serializer.Serialize(path), "SceneSerializer::Serialize reported failure");
		EB_CHECK_MSG(std::filesystem::exists(path), "scene file was not written");
	}

	SceneFixture loaded("Loaded");
	SceneSerializer serializer(loaded.Shared());
	EB_CHECK_MSG(serializer.Deserialize(path), "SceneSerializer::Deserialize reported failure");

	// Identity survives by UUID (the stable key) and by name.
	Entity byId = loaded->GetEntity(savedUUID);
	Entity byName = loaded->GetEntity("Persisted");
	EB_CHECK_MSG(byId.IsValid(), "entity UUID did not survive the round trip");
	EB_EXPECT_MSG(byId == byName, "UUID and name lookups disagree after loading");

	// Transform.
	auto& transform = byId.GetComponent<TransformComponent>();
	EB_EXPECT_VEC3_NEAR(transform.Position, Vector3f(7.0f, -3.0f, 1.5f), 1e-4f);
	EB_EXPECT_EULER_NEAR(transform.Rotation, Vector3f(0.0f, Math::Radians(45.0f), 0.0f), 1e-3f);
	EB_EXPECT_VEC3_NEAR(transform.Scale, Vector3f(2.0f, 0.5f, 1.0f), 1e-4f);

	// Light.
	EB_CHECK(byId.ContainsComponent<PointLightComponent>());
	auto& light = byId.GetComponent<PointLightComponent>();
	EB_EXPECT_VEC3_NEAR(light.Color, Vector3f(0.25f, 0.5f, 0.75f), 1e-4f);
	EB_EXPECT_NEAR(light.Intensity, 33.0f, 1e-3);
	EB_EXPECT_NEAR(light.Radius, 4.5f, 1e-3);

	// Asset handles.
	EB_CHECK(byId.ContainsComponent<StaticMeshComponent>());
	EB_EXPECT_EQ(byId.GetComponent<StaticMeshComponent>().MeshHandle, UUID(Constants::Assets::CubeMeshUUID));
	EB_CHECK(byId.ContainsComponent<MaterialComponent>());
	EB_EXPECT_EQ(byId.GetComponent<MaterialComponent>().MaterialHandle, UUID(Constants::Assets::StandardGeometryMatUUID));

	// Physics authoring data (the runtime Body pointer is deliberately NOT persisted).
	EB_CHECK(byId.ContainsComponent<RigidBodyComponent>());
	auto& rigidBody = byId.GetComponent<RigidBodyComponent>();
	EB_EXPECT(rigidBody.Type == RigidBodyComponent::BodyType::Kinematic);
	EB_EXPECT_NEAR(rigidBody.Mass, 12.5f, 1e-3);
	EB_EXPECT_FALSE(rigidBody.GravityEnabled);
	EB_EXPECT_MSG(rigidBody.Body == nullptr, "a runtime rp3d pointer was written into the scene file");

	// Misc.
	EB_CHECK(byId.ContainsComponent<LifetimeComponent>());
	EB_EXPECT_NEAR(byId.GetComponent<LifetimeComponent>().Lifetime, 9.5f, 1e-3);
	EB_CHECK(byId.ContainsComponent<TextComponent>());
	EB_EXPECT_EQ(byId.GetComponent<TextComponent>().Text, std::string("Hello Ember"));
	EB_EXPECT_VEC4_NEAR(byId.GetComponent<TextComponent>().Color, Vector4f(1.0f, 0.0f, 0.0f, 0.5f), 1e-3f);

	Ember::Test::RemoveTempFile(path);
}

EB_TEST_CASE(Serialization, SceneRoundTripPreservesHierarchy, Integration)
{
	// Relationships are stored as UUIDs, so this is really a test that the UUID graph reconnects.
	// A break here loads every entity as a root and flattens the whole scene.
	const std::string path = TempFile("scene_hierarchy.ebs");
	Ember::Test::RemoveTempFile(path);

	UUID rootUUID, childUUID, grandchildUUID;
	{
		SceneFixture source("HierarchyScene");
		Entity root = MakeEntityAt(*source, "Root", Vector3f(1.0f, 0.0f, 0.0f));
		Entity child = root.AddChild("Child");
		Entity grandchild = child.AddChild("Grandchild");
		child.GetComponent<TransformComponent>().Position = Vector3f(2.0f, 0.0f, 0.0f);
		grandchild.GetComponent<TransformComponent>().Position = Vector3f(4.0f, 0.0f, 0.0f);

		rootUUID = root.GetUUID();
		childUUID = child.GetUUID();
		grandchildUUID = grandchild.GetUUID();

		source.UpdateTransforms();

		SceneSerializer serializer(source.Shared());
		EB_CHECK(serializer.Serialize(path));
	}

	SceneFixture loaded("LoadedHierarchy");
	SceneSerializer serializer(loaded.Shared());
	EB_CHECK(serializer.Deserialize(path));

	Entity root = loaded->GetEntity(rootUUID);
	Entity child = loaded->GetEntity(childUUID);
	Entity grandchild = loaded->GetEntity(grandchildUUID);

	EB_CHECK(root.IsValid());
	EB_CHECK(child.IsValid());
	EB_CHECK(grandchild.IsValid());

	EB_EXPECT(root.IsRootParent());
	EB_EXPECT(child.GetParent() == root);
	EB_EXPECT(grandchild.GetParent() == child);
	EB_EXPECT_EQ(root.GetNumChildren(), (uint32_t)1);
	EB_EXPECT_EQ(child.GetNumChildren(), (uint32_t)1);

	// World transforms must recompose to the same values from the loaded locals.
	loaded.UpdateTransforms();
	EB_EXPECT_VEC3_NEAR(grandchild.GetComponent<TransformComponent>().GetWorldPosition(), Vector3f(7.0f, 0.0f, 0.0f), 1e-3f);

	Ember::Test::RemoveTempFile(path);
}

EB_TEST_CASE(Serialization, DisabledStateSurvivesRoundTrip, Integration)
{
	const std::string path = TempFile("scene_disabled.ebs");
	Ember::Test::RemoveTempFile(path);

	UUID disabledUUID, enabledUUID;
	{
		SceneFixture source("DisabledScene");
		Entity disabled = source->AddEntity("Disabled");
		Entity enabled = source->AddEntity("Enabled");
		disabled.SetActive(false);

		disabledUUID = disabled.GetUUID();
		enabledUUID = enabled.GetUUID();

		SceneSerializer serializer(source.Shared());
		EB_CHECK(serializer.Serialize(path));
	}

	SceneFixture loaded("LoadedDisabled");
	SceneSerializer serializer(loaded.Shared());
	EB_CHECK(serializer.Deserialize(path));

	Entity disabled = loaded->GetEntity(disabledUUID);
	Entity enabled = loaded->GetEntity(enabledUUID);
	EB_CHECK(disabled.IsValid());
	EB_CHECK(enabled.IsValid());

	EB_EXPECT_MSG(!disabled.IsActive(), "a disabled entity came back enabled - it will run and render on load");
	EB_EXPECT(enabled.IsActive());

	Ember::Test::RemoveTempFile(path);
}

EB_TEST_CASE(Serialization, DeserializeClearsThePreviousScene, Integration)
{
	// Loading into a populated scene must REPLACE its contents, not merge into them. Merging leaves
	// the previous level's entities alive behind the new one.
	const std::string path = TempFile("scene_replace.ebs");
	Ember::Test::RemoveTempFile(path);

	{
		SceneFixture source("SmallScene");
		source->AddEntity("OnlyEntity");
		SceneSerializer serializer(source.Shared());
		EB_CHECK(serializer.Serialize(path));
	}

	SceneFixture target("PrePopulated");
	for (int i = 0; i < 5; ++i)
		target->AddEntity("Stale" + std::to_string(i));
	EB_CHECK_EQ(target->GetAllEntities().size(), (size_t)5);

	SceneSerializer serializer(target.Shared());
	EB_CHECK(serializer.Deserialize(path));

	EB_EXPECT_EQ(target->GetAllEntities().size(), (size_t)1);
	EB_EXPECT((target->GetEntity("OnlyEntity")).IsValid());
	EB_EXPECT_FALSE((target->GetEntity("Stale0")).IsValid());

	Ember::Test::RemoveTempFile(path);
}

EB_TEST_CASE(Serialization, DeserializeMissingFileFailsCleanly, Integration)
{
	// A missing scene file must return false, not crash and not half-load. The editor surfaces this
	// as an error toast; a hard failure here would take the whole app down on a bad path.
	SceneFixture scene("MissingFileScene");
	scene->AddEntity("Survivor");

	SceneSerializer serializer(scene.Shared());
	EB_EXPECT_FALSE(serializer.Deserialize(TempFile("this_file_does_not_exist.ebs")));

	// The existing scene must be untouched by the failed load.
	EB_EXPECT_EQ(scene->GetAllEntities().size(), (size_t)1);
}

EB_TEST_CASE(Serialization, SavingEverySceneSkipsUnopenedOnes, Integration)
{
	// Regression: EditorLayer::SaveProject serialized every Scene in the AssetManager. A scene that
	// has not been opened is registered as an empty placeholder (AssetManager::Load<Scene> reads no
	// entities), so saving the project overwrote every unopened scene file with an empty scene.
	const std::string path = std::filesystem::absolute(TempFile("scene_unopened.ebs")).string();
	Ember::Test::RemoveTempFile(path);

	{
		SceneFixture authored("UnopenedScene");
		authored->AddEntity("KeepMe");
		authored->AddEntity("KeepMeToo");

		SceneSerializer serializer(authored.Shared());
		EB_CHECK(serializer.Serialize(path));
		EB_EXPECT_MSG(authored->IsLoaded(), "a scene whose contents were just written should count as loaded");
	}

	// This is what opening a project does for every scene named in the asset registry.
	auto placeholder = Assets().Load<Scene>(path, false);
	EB_CHECK(placeholder != nullptr);
	EB_EXPECT_MSG(!placeholder->IsLoaded(), "a registered but unopened scene must not report as loaded");
	EB_EXPECT_EQ(placeholder->GetAllEntities().size(), (size_t)0);

	// The save-every-scene loop, with the guard SaveProject applies.
	for (auto& scene : Assets().GetAssetsOfType<Scene>())
	{
		if (!scene->IsEngineAsset() && !scene->GetFilePath().empty() && scene->IsLoaded())
			SceneSerializer(scene).Serialize(scene->GetFilePath());
	}

	SceneFixture reloaded("Reloaded");
	SceneSerializer reloadSerializer(reloaded.Shared());
	EB_CHECK(reloadSerializer.Deserialize(path));
	EB_EXPECT_MSG(reloaded->GetEntity("KeepMe").IsValid(), "saving the project emptied an unopened scene file");
	EB_EXPECT_EQ(reloaded->GetAllEntities().size(), (size_t)2);

	// Reading the contents in is what makes the placeholder safe to write back out.
	SceneSerializer placeholderSerializer(placeholder);
	EB_CHECK(placeholderSerializer.Deserialize(path));
	EB_EXPECT_MSG(placeholder->IsLoaded(), "deserializing a scene must mark it loaded");
	EB_EXPECT_EQ(placeholder->GetAllEntities().size(), (size_t)2);

	Assets().RemoveAsset(placeholder->GetUUID());	// RemoveAsset deletes the file too
	Ember::Test::RemoveTempFile(path);
}

//////////////////////////////////////////////////////////////////////////
// Prefabs
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Serialization, PrefabRoundTripRemapsUuids, Integration)
{
	// A prefab is a serialized subtree that can be instantiated many times, so every instance needs
	// FRESH UUIDs while its internal parent/child references stay self-consistent. If the remap
	// leaked the authored UUIDs, two instances would alias each other's children.
	const std::string path = TempFile("roundtrip.eprefab");
	Ember::Test::RemoveTempFile(path);

	SceneFixture scene("PrefabScene");
	Entity root = MakeEntityAt(*scene, "PrefabRoot", Vector3f(1.0f, 2.0f, 3.0f));
	root.AttachComponent<PointLightComponent>(Vector3f(0.0f, 1.0f, 0.0f), 17.0f, 2.0f);
	Entity child = root.AddChild("PrefabChild");
	child.GetComponent<TransformComponent>().Position = Vector3f(0.0f, 1.0f, 0.0f);
	scene.UpdateTransforms();

	const UUID authoredRootUUID = root.GetUUID();
	const UUID authoredChildUUID = child.GetUUID();

	SceneSerializer serializer(scene.Shared());
	EB_CHECK_MSG(serializer.SerializePrefab(root, path), "SerializePrefab reported failure");
	EB_CHECK(std::filesystem::exists(path));

	// Build the Prefab asset directly: Scene::CreatePrefab additionally writes the project asset
	// registry, which needs an active Project this test has no reason to stand up.
	auto prefab = SharedPtr<Prefab>::Create(UUID(), "EmberTest_Prefab", path);
	EB_CHECK_MSG(!prefab->YAMLData.empty(), "the Prefab asset read no YAML back from disk");

	Entity instance = serializer.DeserializePrefab(prefab, /*preserveUUIDs*/ false);
	EB_CHECK_MSG(instance.IsValid(), "DeserializePrefab returned an invalid entity");

	// Fresh identity...
	EB_EXPECT_NE(instance.GetUUID(), authoredRootUUID);

	// ...same data.
	EB_CHECK(instance.ContainsComponent<PointLightComponent>());
	EB_EXPECT_NEAR(instance.GetComponent<PointLightComponent>().Intensity, 17.0f, 1e-3);
	EB_EXPECT_VEC3_NEAR(instance.GetComponent<TransformComponent>().Position, Vector3f(1.0f, 2.0f, 3.0f), 1e-4f);

	// ...and an internally consistent subtree pointing at the NEW child, not the authored one.
	EB_CHECK_EQ(instance.GetNumChildren(), (uint32_t)1);
	Entity instanceChild = instance.GetChildByName("PrefabChild");
	EB_CHECK(instanceChild.IsValid());
	EB_EXPECT_NE(instanceChild.GetUUID(), authoredChildUUID);
	EB_EXPECT(instanceChild.GetParent() == instance);

	// A second instantiation must not collide with the first.
	Entity secondInstance = serializer.DeserializePrefab(prefab, false);
	EB_CHECK(secondInstance.IsValid());
	EB_EXPECT_NE(secondInstance.GetUUID(), instance.GetUUID());
	Entity secondChild = secondInstance.GetChildByName("PrefabChild");
	EB_CHECK(secondChild.IsValid());
	EB_EXPECT_NE(secondChild.GetUUID(), instanceChild.GetUUID());

	Ember::Test::RemoveTempFile(path);
}

// Application::GetAssetManager() returns a reference, so binding it with plain `auto` copies the
// whole registry. Assets loaded through such a copy register into a temporary that dies at end of
// scope, and the caller is handed a UUID the real manager asserts on. ScriptGenerator shipped that
// bug; making the type non-copyable turns it into a compile error instead of a runtime assert.
EB_TEST_CASE(Assets, AssetManagerIsNonCopyable, Unit)
{
	EB_EXPECT_FALSE(std::is_copy_constructible_v<AssetManager>);
	EB_EXPECT_FALSE(std::is_copy_assignable_v<AssetManager>);
}

EB_TEST_CASE(Assets, GeneratedScriptIsRetrievableByUUID, Integration)
{
	Ember::Test::RequireDefaultAssets();

	const std::string path = Ember::Test::TempFile("generated_script.lua");
	Ember::Test::RemoveTempFile(path);

	{
		std::ofstream out(path);
		out << "local Generated = {}\nfunction Generated:OnClick(entity)\nend\nreturn Generated\n";
	}

	auto& assetManager = Application::Instance().GetAssetManager();
	auto script = assetManager.Load<Script>(path, false);
	EB_CHECK_MSG(script, "Load<Script> returned null");

	// The whole point: the UUID handed back must resolve in the manager the rest of the editor
	// queries, not in a copy that has already been destroyed.
	EB_CHECK_MSG(assetManager.ContainsAsset(script->GetUUID()),
		"the loaded script did not register in the live AssetManager");
	EB_EXPECT_EQ(assetManager.GetAsset<Script>(script->GetUUID())->GetUUID(), script->GetUUID());

	Ember::Test::RemoveTempFile(path);
}
