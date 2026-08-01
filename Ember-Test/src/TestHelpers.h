#pragma once

// Engine-aware test utilities: approximate comparisons for Ember's math types, and RAII fixtures that
// stand a Scene up (and tear it back down) correctly. TestFramework.h stays engine-free; everything
// that needs <Ember.h> lives here.
//
// WHY THE FIXTURES MATTER
// -----------------------
// Several engine systems keep per-scene state that is created on attach and only reset on the *next*
// attach — physics most of all: PhysicsSystem::OnSceneAttach() destroys the whole rp3d world and
// rebuilds it from the scene's components. Get the ordering wrong and tests silently measure garbage:
//
//   * Rigid bodies are spawned from TransformComponent::WorldTransform, which is only populated by
//     TransformSystem. Attach physics before running transforms and every body spawns at the origin.
//   * Bodies created for a previous scene are destroyed when the next scene attaches, so a test that
//     holds an rp3d pointer across a fixture boundary is reading freed memory.
//
// PhysicsSceneFixture encodes the correct order (build -> transform -> attach) so individual tests
// can't get it wrong.

#include <Ember.h>

#include "TestFramework.h"

#include <filesystem>
#include <string>

namespace Ember::Test {

	//////////////////////////////////////////////////////////////////////////
	// Common constants / shorthands
	//////////////////////////////////////////////////////////////////////////

	// One 60 Hz frame. PhysicsSystem's internal accumulator ticks at PhysicsSettings::UpdateRate
	// (60 by default), so stepping with exactly this value gives one simulation step per call.
	inline TimeStep FixedStep() { return TimeStep(1.0f / 60.0f); }

	template<typename T>
	inline SharedPtr<T> Sys() { return Application::Instance().GetSystem<T>(); }

	inline AssetManager& Assets() { return Application::Instance().GetAssetManager(); }

	//////////////////////////////////////////////////////////////////////////
	// Formatting helpers (used by the comparison macros below)
	//////////////////////////////////////////////////////////////////////////

	inline std::string ToString(const Vector2f& v)
	{
		char buffer[128];
		std::snprintf(buffer, sizeof(buffer), "(%.4f, %.4f)", v.x, v.y);
		return std::string(buffer);
	}

	inline std::string ToString(const Vector3f& v)
	{
		char buffer[160];
		std::snprintf(buffer, sizeof(buffer), "(%.4f, %.4f, %.4f)", v.x, v.y, v.z);
		return std::string(buffer);
	}

	inline std::string ToString(const Vector4f& v)
	{
		char buffer[192];
		std::snprintf(buffer, sizeof(buffer), "(%.4f, %.4f, %.4f, %.4f)", v.x, v.y, v.z, v.w);
		return std::string(buffer);
	}

	inline std::string ToString(const Quaternion& q)
	{
		char buffer[192];
		std::snprintf(buffer, sizeof(buffer), "(x %.4f, y %.4f, z %.4f, w %.4f)", q.x, q.y, q.z, q.w);
		return std::string(buffer);
	}

	inline std::string ToString(const Matrix4f& m)
	{
		std::string out = "\n";
		for (int row = 0; row < 4; ++row)
		{
			char buffer[192];
			// GLM is column-major: m[col][row].
			std::snprintf(buffer, sizeof(buffer), "           [%9.4f %9.4f %9.4f %9.4f]\n",
				m[0][row], m[1][row], m[2][row], m[3][row]);
			out += buffer;
		}
		return out;
	}

	//////////////////////////////////////////////////////////////////////////
	// Approximate comparisons
	//////////////////////////////////////////////////////////////////////////

	inline bool NearlyEqual(float a, float b, float epsilon)
	{
		return std::abs(a - b) <= epsilon;
	}

	inline bool NearlyEqual(const Vector2f& a, const Vector2f& b, float epsilon)
	{
		return NearlyEqual(a.x, b.x, epsilon) && NearlyEqual(a.y, b.y, epsilon);
	}

	inline bool NearlyEqual(const Vector3f& a, const Vector3f& b, float epsilon)
	{
		return NearlyEqual(a.x, b.x, epsilon) && NearlyEqual(a.y, b.y, epsilon) && NearlyEqual(a.z, b.z, epsilon);
	}

	inline bool NearlyEqual(const Vector4f& a, const Vector4f& b, float epsilon)
	{
		return NearlyEqual(a.x, b.x, epsilon) && NearlyEqual(a.y, b.y, epsilon)
			&& NearlyEqual(a.z, b.z, epsilon) && NearlyEqual(a.w, b.w, epsilon);
	}

	inline bool NearlyEqual(const Matrix4f& a, const Matrix4f& b, float epsilon)
	{
		for (int col = 0; col < 4; ++col)
			for (int row = 0; row < 4; ++row)
				if (!NearlyEqual(a[col][row], b[col][row], epsilon))
					return false;
		return true;
	}

	// q and -q encode the SAME rotation, so a raw component compare produces false failures. Compare
	// the absolute dot product against 1 instead: that is 1 for equal rotations under either sign.
	inline bool NearlyEqualRotation(const Quaternion& a, const Quaternion& b, float epsilon)
	{
		const float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
		return std::abs(std::abs(dot) - 1.0f) <= epsilon;
	}

	// Euler triples are not unique for a given orientation, so comparing them component-wise is a
	// classic source of flaky animation/transform tests. Convert to quaternions and compare rotations.
	inline bool NearlyEqualEuler(const Vector3f& eulerA, const Vector3f& eulerB, float epsilon)
	{
		return NearlyEqualRotation(Math::ToQuaternion(eulerA), Math::ToQuaternion(eulerB), epsilon);
	}

	//////////////////////////////////////////////////////////////////////////
	// Scene fixtures
	//////////////////////////////////////////////////////////////////////////

	// A bare scene: entity storage plus whatever systems the test drives by hand. No system state is
	// attached, so this is the cheapest fixture and the right default for ECS / serialization tests.
	class SceneFixture
	{
	public:
		explicit SceneFixture(const std::string& name = "TestScene")
			: m_Scene(SharedPtr<Scene>::Create(name, ""))
		{
		}

		virtual ~SceneFixture()
		{
			// Nothing to unwind: ~Scene() does not touch any system, and every system that holds
			// per-scene state keys it off the registry that dies with the scene. Physics is the one
			// exception and PhysicsSceneFixture handles it.
		}

		SceneFixture(const SceneFixture&) = delete;
		SceneFixture& operator=(const SceneFixture&) = delete;

		Scene* operator->() const { return m_Scene.Ptr(); }
		Scene& operator*() const { return *m_Scene.Ptr(); }
		Scene* Ptr() const { return m_Scene.Ptr(); }
		const SharedPtr<Scene>& Shared() const { return m_Scene; }

		// Recomputes every WorldTransform from the local TRS values. Required before reading
		// GetWorldPosition(), before parenting (SetEntityParent derives the new local transform from
		// the parent's world matrix), and before attaching physics.
		void UpdateTransforms(TimeStep delta = FixedStep())
		{
			Sys<TransformSystem>()->OnUpdate(delta, m_Scene.Ptr());
		}

		// A full editor-style frame: transforms, bone sockets, editor physics sync, UI layout, render,
		// and finally Scene::RemovePendingRemovals().
		//
		// Scene::RemoveEntity() only QUEUES a removal; the queue is private and drained exclusively at
		// the end of OnUpdateEdit / OnUpdateRuntime. So any test that asserts an entity is really gone
		// has to run a real frame - there is no lighter hook. It renders into the window's back buffer,
		// which the runner has already sized via RenderSystem::OnViewportResize.
		void TickEdit(TimeStep delta = FixedStep())
		{
			const Window& window = Application::Instance().GetWindow();

			Camera camera;
			camera.SetPerspective(60.0f, 0.1f, 200.0f);
			camera.SetViewportSize(window.GetWidth(), window.GetHeight());

			RenderPassSettings settings;
			settings.ActiveCamera = &camera;
			settings.CameraTransform = Math::Inverse(
				Math::LookAt(Vector3f(0.0f, 4.0f, 12.0f), Vector3f(0.0f), Vector3f(0.0f, 1.0f, 0.0f)));
			settings.DrawHUD = false;

			m_Scene->OnUpdateEdit(delta, settings);
		}

	protected:
		SharedPtr<Scene> m_Scene;
	};

	// A scene wired into the physics world.
	//
	// Build the scene first, then call Attach(). Attaching runs TransformSystem so world transforms
	// are correct, then PhysicsSystem::OnSceneAttach, whose ConnectAndRetroact hooks spawn an rp3d
	// body/collider for every component already present. Components attached *after* Attach() still
	// get bodies (the hooks stay connected) but are spawned from whatever WorldTransform is current,
	// so call UpdateTransforms() first if you move them.
	class PhysicsSceneFixture : public SceneFixture
	{
	public:
		explicit PhysicsSceneFixture(const std::string& name = "PhysicsTestScene")
			: SceneFixture(name)
		{
		}

		~PhysicsSceneFixture() override
		{
			// Null out every runtime physics pointer before the scene dies. The rp3d objects
			// themselves are owned by the physics world and are freed the next time a scene attaches
			// (RestartPhysicsWorld) or at PhysicsSystem::OnDetach; what matters here is that nothing
			// can dereference a stale Body/Shape afterwards.
			if (m_Attached)
				m_Scene->ResetAllPhysicsState();
		}

		void Attach()
		{
			UpdateTransforms();
			m_Scene->OnAttach();
			m_Attached = true;
		}

		// Advances the simulation the way Scene::OnUpdateRuntime does: physics first, then transforms.
		void Step(int frames = 1, TimeStep delta = FixedStep())
		{
			auto physics = Sys<PhysicsSystem>();
			auto transforms = Sys<TransformSystem>();
			for (int i = 0; i < frames; ++i)
			{
				physics->OnUpdate(delta, m_Scene.Ptr());
				transforms->OnUpdate(delta, m_Scene.Ptr());
			}
		}

		bool IsAttached() const { return m_Attached; }

	private:
		bool m_Attached = false;
	};

	//////////////////////////////////////////////////////////////////////////
	// Entity construction shorthands
	//////////////////////////////////////////////////////////////////////////

	inline Entity MakeEntityAt(Scene& scene, const std::string& name, const Vector3f& position,
		const Vector3f& rotation = Vector3f(0.0f), const Vector3f& scale = Vector3f(1.0f))
	{
		Entity entity = scene.AddEntity(name);
		// AddEntity already attaches a default TransformComponent; re-attaching replaces it in place.
		entity.AttachComponent<TransformComponent>(position, rotation, scale);
		return entity;
	}

	// A static box the world can rest on / raycast against.
	inline Entity MakeStaticBox(Scene& scene, const std::string& name, const Vector3f& position, const Vector3f& size)
	{
		Entity entity = MakeEntityAt(scene, name, position);
		entity.AttachComponent<RigidBodyComponent>(RigidBodyComponent::BodyType::Static);
		entity.AttachComponent<BoxColliderComponent>(size);
		return entity;
	}

	// A dynamic box that gravity acts on.
	inline Entity MakeDynamicBox(Scene& scene, const std::string& name, const Vector3f& position,
		const Vector3f& size = Vector3f(1.0f), float mass = 1.0f)
	{
		Entity entity = MakeEntityAt(scene, name, position);
		entity.AttachComponent<RigidBodyComponent>(RigidBodyComponent::BodyType::Dynamic, mass, true);
		entity.AttachComponent<BoxColliderComponent>(size);
		return entity;
	}

	//////////////////////////////////////////////////////////////////////////
	// Filesystem helpers
	//////////////////////////////////////////////////////////////////////////

	// Scratch directory for files a test writes (serialization round-trips, generated goldens).
	// Relative to the working directory, which premake pins to the workspace root via `debugdir`.
	inline std::filesystem::path TempDir()
	{
		std::filesystem::path dir = "Ember-Test/tmp";
		std::error_code ec;
		std::filesystem::create_directories(dir, ec);
		return dir;
	}

	inline std::string TempFile(const std::string& fileName)
	{
		return (TempDir() / fileName).string();
	}

	// Deletes a scratch file if present. Safe to call when it was never created.
	inline void RemoveTempFile(const std::string& path)
	{
		std::error_code ec;
		std::filesystem::remove(path, ec);
	}

	//////////////////////////////////////////////////////////////////////////
	// Asset helpers
	//////////////////////////////////////////////////////////////////////////

	// AssetManager::GetAsset() fires EB_CORE_ASSERT (which is __debugbreak() in Debug) when the asset
	// is missing, so a test must never call it speculatively. Use this instead - it returns null.
	template<IsCoreAsset T>
	inline SharedPtr<T> TryGetAsset(UUID uuid)
	{
		if (!Assets().ContainsAsset(uuid))
			return nullptr;
		return Assets().GetAsset<T>(uuid);
	}

	template<IsCoreAsset T>
	inline SharedPtr<T> TryGetAsset(const std::string& name)
	{
		if (!Assets().ContainsAssetWithName(name))
			return nullptr;
		return Assets().GetAsset<T>(name);
	}

	// Skips (rather than fails) the calling test when a default engine asset is absent, which happens
	// when the executable is run from the wrong working directory. That is an environment problem, not
	// an engine regression, and failing on it would bury real defects in noise.
	inline void RequireDefaultAssets()
	{
		if (!Assets().ContainsAsset(UUID(Constants::Assets::CubeMeshUUID)))
			ReportSkip("default engine assets not loaded - is the working directory the workspace root?");
	}

} // namespace Ember::Test

//////////////////////////////////////////////////////////////////////////
// Engine-typed comparison macros
//////////////////////////////////////////////////////////////////////////

#define EB_EXPECT_VEC2_NEAR(a, b, eps) do {                                                             \
		const ::Ember::Vector2f _ebA = (a), _ebB = (b);                                                 \
		if (!::Ember::Test::NearlyEqual(_ebA, _ebB, (float)(eps)))                                      \
			::Ember::Test::ReportSoftFail("EXPECT_VEC2_NEAR(" #a ", " #b ")", __FILE__, __LINE__,        \
				::Ember::Test::ToString(_ebA) + " vs " + ::Ember::Test::ToString(_ebB));                 \
	} while (0)

#define EB_EXPECT_VEC3_NEAR(a, b, eps) do {                                                             \
		const ::Ember::Vector3f _ebA = (a), _ebB = (b);                                                 \
		if (!::Ember::Test::NearlyEqual(_ebA, _ebB, (float)(eps)))                                      \
			::Ember::Test::ReportSoftFail("EXPECT_VEC3_NEAR(" #a ", " #b ")", __FILE__, __LINE__,        \
				::Ember::Test::ToString(_ebA) + " vs " + ::Ember::Test::ToString(_ebB));                 \
	} while (0)

#define EB_CHECK_VEC3_NEAR(a, b, eps) do {                                                              \
		const ::Ember::Vector3f _ebA = (a), _ebB = (b);                                                 \
		if (!::Ember::Test::NearlyEqual(_ebA, _ebB, (float)(eps)))                                      \
			::Ember::Test::ReportFail("CHECK_VEC3_NEAR(" #a ", " #b ")", __FILE__, __LINE__,             \
				::Ember::Test::ToString(_ebA) + " vs " + ::Ember::Test::ToString(_ebB));                 \
	} while (0)

#define EB_EXPECT_VEC4_NEAR(a, b, eps) do {                                                             \
		const ::Ember::Vector4f _ebA = (a), _ebB = (b);                                                 \
		if (!::Ember::Test::NearlyEqual(_ebA, _ebB, (float)(eps)))                                      \
			::Ember::Test::ReportSoftFail("EXPECT_VEC4_NEAR(" #a ", " #b ")", __FILE__, __LINE__,        \
				::Ember::Test::ToString(_ebA) + " vs " + ::Ember::Test::ToString(_ebB));                 \
	} while (0)

#define EB_EXPECT_MAT4_NEAR(a, b, eps) do {                                                             \
		const ::Ember::Matrix4f _ebA = (a), _ebB = (b);                                                 \
		if (!::Ember::Test::NearlyEqual(_ebA, _ebB, (float)(eps)))                                      \
			::Ember::Test::ReportSoftFail("EXPECT_MAT4_NEAR(" #a ", " #b ")", __FILE__, __LINE__,        \
				::Ember::Test::ToString(_ebA) + "  vs  " + ::Ember::Test::ToString(_ebB));               \
	} while (0)

// Compares ORIENTATIONS, not components: q and -q pass, and equivalent Euler triples pass.
#define EB_EXPECT_ROTATION_NEAR(a, b, eps) do {                                                         \
		const ::Ember::Quaternion _ebA = (a), _ebB = (b);                                               \
		if (!::Ember::Test::NearlyEqualRotation(_ebA, _ebB, (float)(eps)))                               \
			::Ember::Test::ReportSoftFail("EXPECT_ROTATION_NEAR(" #a ", " #b ")", __FILE__, __LINE__,    \
				::Ember::Test::ToString(_ebA) + " vs " + ::Ember::Test::ToString(_ebB));                 \
	} while (0)

#define EB_EXPECT_EULER_NEAR(a, b, eps) do {                                                            \
		const ::Ember::Vector3f _ebA = (a), _ebB = (b);                                                 \
		if (!::Ember::Test::NearlyEqualEuler(_ebA, _ebB, (float)(eps)))                                  \
			::Ember::Test::ReportSoftFail("EXPECT_EULER_NEAR(" #a ", " #b ")", __FILE__, __LINE__,       \
				::Ember::Test::ToString(_ebA) + " vs " + ::Ember::Test::ToString(_ebB));                 \
	} while (0)
