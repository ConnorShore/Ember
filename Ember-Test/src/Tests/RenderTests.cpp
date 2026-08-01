// RENDERING TESTS (non-pixel)
// ---------------------------
// Camera projections, frustum extraction, culling and the visibility system. These assert on the
// MATHS and the BOOKKEEPING that decide what gets drawn, which is testable without comparing pixels;
// the golden-image tests in VisualTests.cpp cover the actual output.
//
// Culling is a particularly good candidate for automated testing: when it is too aggressive, objects
// pop out of existence at the edge of the screen - a bug that is intermittent, position-dependent,
// and almost impossible to reproduce deliberately by hand.

#include <Ember.h>

// VisibilitySystem is not re-exported from Ember.h, unlike the other systems.
#include "Ember/ECS/System/VisibilitySystem.h"

#include "TestFramework.h"
#include "TestHelpers.h"

#include <string>
#include <utility>
#include <vector>

using namespace Ember;
using Ember::Test::Type::Integration;
using Ember::Test::Type::Unit;
using Ember::Test::MakeEntityAt;
using Ember::Test::SceneFixture;
using Ember::Test::Sys;

namespace {

	// View-projection for a camera at `eye` looking at `target`.
	Matrix4f MakeViewProjection(const Vector3f& eye, const Vector3f& target,
		float fovDegrees = 60.0f, float aspect = 16.0f / 9.0f, float nearClip = 0.1f, float farClip = 100.0f)
	{
		const Matrix4f projection = Math::Perspective(fovDegrees, aspect, nearClip, farClip);
		const Matrix4f view = Math::LookAt(eye, target, Vector3f(0.0f, 1.0f, 0.0f));
		return projection * view;
	}

	// Axis-aligned box of the given half-extent around `centre`.
	AABB MakeBox(const Vector3f& centre, float halfExtent = 0.5f)
	{
		return AABB{ centre - Vector3f(halfExtent), centre + Vector3f(halfExtent) };
	}

	// A cube entity the visibility/render systems will consider renderable.
	Entity MakeRenderableCube(Scene& scene, const std::string& name, const Vector3f& position)
	{
		Entity entity = MakeEntityAt(scene, name, position);
		entity.AttachComponent<StaticMeshComponent>(UUID(Constants::Assets::CubeMeshUUID));
		entity.AttachComponent<MaterialComponent>(UUID(Constants::Assets::StandardGeometryMatUUID));
		return entity;
	}

	Entity MakeActiveCamera(Scene& scene, const Vector3f& position, const Vector3f& eulerRotation = Vector3f(0.0f))
	{
		Entity entity = MakeEntityAt(scene, "Camera", position, eulerRotation);
		auto& cameraComponent = entity.AttachComponent<CameraComponent>();
		cameraComponent.IsActive = true;
		cameraComponent.Camera.SetPerspective(60.0f, 0.1f, 200.0f);
		cameraComponent.Camera.SetViewportSize(1280, 720);
		return entity;
	}

} // namespace

//////////////////////////////////////////////////////////////////////////
// Camera
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Render, CameraProjectionSwitching, Unit)
{
	// CONTRACT: SetPerspective/SetOrthographic only store that projection's PARAMETERS and rebuild
	// the matrix; they do NOT change which projection is active. Selecting the mode is a separate
	// call, SetProjectionType(). Camera defaults to Perspective.
	//
	// That is a genuine footgun - calling SetOrthographic() alone looks like it should switch the
	// camera and silently does nothing visible - but it is the engine's actual behaviour, so this
	// test pins it down rather than asserting what it "ought" to be.
	Camera camera;
	EB_EXPECT_MSG(camera.GetProjectionType() == Camera::ProjectionType::Perspective,
		"a default-constructed Camera should be perspective");

	camera.SetPerspective(60.0f, 0.5f, 250.0f);
	EB_EXPECT_NEAR(camera.GetNearClip(), 0.5f, 1e-5);
	EB_EXPECT_NEAR(camera.GetFarClip(), 250.0f, 1e-4);
	EB_EXPECT_NEAR(camera.GetPerspectiveProps().FieldOfView, 60.0f, 1e-5);

	// Storing orthographic props does not switch modes on its own...
	camera.SetOrthographic(10.0f, -5.0f, 5.0f);
	EB_EXPECT_MSG(camera.GetProjectionType() == Camera::ProjectionType::Perspective,
		"SetOrthographic unexpectedly changed the active projection type");
	EB_EXPECT_NEAR(camera.GetOrthographicProps().Size, 10.0f, 1e-5);
	EB_EXPECT_MSG(std::abs(camera.GetNearClip() - 0.5f) < 1e-5f,
		"the clip getters should still report the PERSPECTIVE planes while perspective is active");

	// ...SetProjectionType is what actually selects it.
	camera.SetProjectionType(Camera::ProjectionType::Orthographic);
	EB_EXPECT_MSG(camera.GetProjectionType() == Camera::ProjectionType::Orthographic,
		"SetProjectionType did not switch the projection type");
	EB_EXPECT_NEAR(camera.GetNearClip(), -5.0f, 1e-5);
	EB_EXPECT_NEAR(camera.GetFarClip(), 5.0f, 1e-5);

	// Switching back reports the perspective planes again - the two parameter sets are independent.
	camera.SetProjectionType(Camera::ProjectionType::Perspective);
	camera.SetPerspective(90.0f, 1.0f, 500.0f);
	EB_EXPECT_NEAR(camera.GetNearClip(), 1.0f, 1e-5);
	EB_EXPECT_NEAR(camera.GetFarClip(), 500.0f, 1e-4);
	EB_EXPECT_NEAR(camera.GetOrthographicProps().Size, 10.0f, 1e-5); // untouched
}

EB_TEST_CASE(Render, CameraViewportDrivesAspectRatio, Unit)
{
	Camera camera;
	camera.SetPerspective(60.0f, 0.1f, 100.0f);
	camera.SetViewportSize(1920, 1080);

	EB_EXPECT_NEAR(camera.GetAspectRatio(), 1920.0f / 1080.0f, 1e-4);
	EB_EXPECT_VEC2_NEAR(camera.GetViewportSize(), Vector2f(1920.0f, 1080.0f), 1e-4f);

	// A resize must be reflected immediately - a stale aspect ratio stretches the whole image.
	camera.SetViewportSize(800, 800);
	EB_EXPECT_NEAR(camera.GetAspectRatio(), 1.0f, 1e-5);
	EB_EXPECT_VEC2_NEAR(camera.GetViewportSize(), Vector2f(800.0f, 800.0f), 1e-4f);
}

EB_TEST_CASE(Render, PerspectiveProjectionMapsNearAndFarPlanes, Unit)
{
	// Points on the near and far planes must land on the -1 and +1 depth boundaries in OpenGL's
	// clip space. If they don't, everything is either z-fighting or clipped away.
	const float nearClip = 0.5f, farClip = 100.0f;
	const Matrix4f projection = Math::Perspective(60.0f, 1.0f, nearClip, farClip);

	const Vector4f nearPoint = projection * Vector4f(0.0f, 0.0f, -nearClip, 1.0f);
	const Vector4f farPoint = projection * Vector4f(0.0f, 0.0f, -farClip, 1.0f);

	EB_CHECK(std::abs(nearPoint.w) > 1e-6f);
	EB_CHECK(std::abs(farPoint.w) > 1e-6f);

	EB_EXPECT_NEAR(nearPoint.z / nearPoint.w, -1.0f, 1e-4);
	EB_EXPECT_NEAR(farPoint.z / farPoint.w, 1.0f, 1e-4);
}

//////////////////////////////////////////////////////////////////////////
// Frustum culling
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Render, FrustumAcceptsBoxesInFrontAndRejectsTheRest, Unit)
{
	// Camera at the origin looking down -Z.
	const Matrix4f viewProjection = MakeViewProjection(Vector3f(0.0f), Vector3f(0.0f, 0.0f, -1.0f));
	const Frustum frustum(viewProjection);

	// Directly ahead, well inside the frustum.
	{
		const AABB box = MakeBox(Vector3f(0.0f, 0.0f, -10.0f));
		EB_EXPECT_MSG(frustum.IsBoxVisible(box.WorldMin, box.WorldMax), "a box directly in front of the camera was culled");
	}

	// Behind the camera.
	{
		const AABB box = MakeBox(Vector3f(0.0f, 0.0f, 10.0f));
		EB_EXPECT_MSG(!frustum.IsBoxVisible(box.WorldMin, box.WorldMax), "a box behind the camera was not culled");
	}

	// Beyond the far plane (100).
	{
		const AABB box = MakeBox(Vector3f(0.0f, 0.0f, -500.0f));
		EB_EXPECT_MSG(!frustum.IsBoxVisible(box.WorldMin, box.WorldMax), "a box beyond the far plane was not culled");
	}

	// Far off to the side.
	{
		const AABB box = MakeBox(Vector3f(500.0f, 0.0f, -10.0f));
		EB_EXPECT_MSG(!frustum.IsBoxVisible(box.WorldMin, box.WorldMax), "a box far outside the horizontal FOV was not culled");
	}

	// Far above.
	{
		const AABB box = MakeBox(Vector3f(0.0f, 500.0f, -10.0f));
		EB_EXPECT_MSG(!frustum.IsBoxVisible(box.WorldMin, box.WorldMax), "a box far outside the vertical FOV was not culled");
	}
}

EB_TEST_CASE(Render, FrustumKeepsBoxesThatOnlyPartlyOverlap, Unit)
{
	// The test is against the whole box, not its centre. A box whose centre is outside the frustum
	// but which still pokes into view must be KEPT - culling it is the classic "objects vanish at
	// the edge of the screen" bug.
	const Matrix4f viewProjection = MakeViewProjection(Vector3f(0.0f), Vector3f(0.0f, 0.0f, -1.0f));
	const Frustum frustum(viewProjection);

	// A very wide slab centred far to the right, still reaching across the view axis.
	const AABB straddling{ Vector3f(-1.0f, -1.0f, -11.0f), Vector3f(1000.0f, 1.0f, -9.0f) };
	EB_EXPECT_MSG(frustum.IsBoxVisible(straddling.WorldMin, straddling.WorldMax),
		"a box that straddles the frustum boundary was culled");

	// A huge box enclosing the camera entirely is trivially visible.
	const AABB enclosing{ Vector3f(-1000.0f), Vector3f(1000.0f) };
	EB_EXPECT_MSG(frustum.IsBoxVisible(enclosing.WorldMin, enclosing.WorldMax),
		"a box containing the camera was culled");
}

EB_TEST_CASE(Render, FrustumFollowsCameraOrientation, Unit)
{
	// Same box, two cameras: the one looking at it keeps it, the one looking away culls it.
	const AABB box = MakeBox(Vector3f(0.0f, 0.0f, -10.0f));

	const Frustum lookingAt(MakeViewProjection(Vector3f(0.0f), Vector3f(0.0f, 0.0f, -1.0f)));
	EB_EXPECT(lookingAt.IsBoxVisible(box.WorldMin, box.WorldMax));

	const Frustum lookingAway(MakeViewProjection(Vector3f(0.0f), Vector3f(0.0f, 0.0f, 1.0f)));
	EB_EXPECT_FALSE(lookingAway.IsBoxVisible(box.WorldMin, box.WorldMax));
}

EB_TEST_CASE(Render, GetEntitiesInFrustumFiltersTheList, Unit)
{
	const Matrix4f viewProjection = MakeViewProjection(Vector3f(0.0f), Vector3f(0.0f, 0.0f, -1.0f));

	std::vector<std::pair<EntityID, AABB>> candidates;
	candidates.emplace_back((EntityID)1, MakeBox(Vector3f(0.0f, 0.0f, -10.0f)));  // visible
	candidates.emplace_back((EntityID)2, MakeBox(Vector3f(0.0f, 0.0f, 10.0f)));   // behind
	candidates.emplace_back((EntityID)3, MakeBox(Vector3f(0.0f, 0.0f, -20.0f)));  // visible
	candidates.emplace_back((EntityID)4, MakeBox(Vector3f(900.0f, 0.0f, -10.0f))); // off to the side

	std::vector<EntityID> visible;
	GetEntitiesInFrustum(candidates, viewProjection, visible);

	EB_CHECK_EQ(visible.size(), (size_t)2);
	EB_EXPECT_EQ(visible[0], (EntityID)1);
	EB_EXPECT_EQ(visible[1], (EntityID)3);
}

//////////////////////////////////////////////////////////////////////////
// Renderable bounds
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Render, RenderableAabbsFollowTheWorldTransform, Integration)
{
	// Bounds are the mesh's local bounds pushed through the world matrix, so translating and
	// scaling an entity must move and grow its AABB. A stale AABB culls a moving object as if it
	// were still at its old position.
	Ember::Test::RequireDefaultAssets();

	SceneFixture scene("AabbScene");
	Entity cube = MakeRenderableCube(*scene, "Cube", Vector3f(10.0f, 0.0f, 0.0f));
	cube.GetComponent<TransformComponent>().Scale = Vector3f(4.0f);
	scene.UpdateTransforms();

	std::vector<std::pair<EntityID, AABB>> renderables;
	VisibilitySystem::GatherRenderableAABBs(scene.Ptr(), renderables);

	EB_CHECK_MSG(renderables.size() == 1, "expected exactly one renderable entity, got "
		+ std::to_string(renderables.size()));
	EB_EXPECT_EQ(renderables[0].first, cube.GetEntityHandle());

	const AABB& box = renderables[0].second;
	const Vector3f centre = (box.WorldMin + box.WorldMax) * 0.5f;
	const Vector3f extents = (box.WorldMax - box.WorldMin) * 0.5f;

	// Centred on the entity...
	EB_EXPECT_VEC3_NEAR(centre, Vector3f(10.0f, 0.0f, 0.0f), 1e-2f);
	// ...and grown by the scale (a unit cube at 4x is 4 units across, half-extent 2).
	EB_EXPECT_NEAR(extents.x, 2.0f, 0.1);
	EB_EXPECT_GT(extents.y, 0.0f);
	EB_EXPECT_GT(extents.z, 0.0f);

	// Entities with no material are not renderable and must not appear.
	Entity meshOnly = MakeEntityAt(*scene, "MeshOnly", Vector3f(0.0f));
	meshOnly.AttachComponent<StaticMeshComponent>(UUID(Constants::Assets::CubeMeshUUID));
	scene.UpdateTransforms();

	VisibilitySystem::GatherRenderableAABBs(scene.Ptr(), renderables);
	EB_EXPECT_MSG(renderables.size() == 1, "an entity without a MaterialComponent was treated as renderable");
}

//////////////////////////////////////////////////////////////////////////
// Visibility system
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Render, VisibilityFailsSafeWithNoCamera, Integration)
{
	// With no active camera the system cannot prove anything is off screen, so EVERYTHING must
	// report visible. Failing open here is deliberate: the alternative is freezing objects that
	// are actually on screen.
	Ember::Test::RequireDefaultAssets();

	auto visibility = Sys<VisibilitySystem>();
	SceneFixture scene("VisibilityNoCameraScene");
	visibility->OnSceneDetach(scene.Ptr()); // drop any state left by an earlier test

	Entity cube = MakeRenderableCube(*scene, "Cube", Vector3f(0.0f, 0.0f, -10.0f));
	scene.UpdateTransforms();

	visibility->OnUpdate(Ember::Test::FixedStep(), scene.Ptr());

	EB_EXPECT_MSG(visibility->IsVisible(cube.GetEntityHandle()),
		"with no active camera every entity must report visible");
	EB_EXPECT(visibility->IsRelevant(cube.GetEntityHandle()));

	visibility->OnSceneDetach(scene.Ptr());
}

EB_TEST_CASE(Render, VisibilityCullsEntitiesBehindTheCamera, Integration)
{
	Ember::Test::RequireDefaultAssets();

	auto visibility = Sys<VisibilitySystem>();
	SceneFixture scene("VisibilityScene");
	visibility->OnSceneDetach(scene.Ptr());

	MakeActiveCamera(*scene, Vector3f(0.0f)); // at the origin, looking down -Z
	Entity inView = MakeRenderableCube(*scene, "InView", Vector3f(0.0f, 0.0f, -20.0f));
	Entity behind = MakeRenderableCube(*scene, "Behind", Vector3f(0.0f, 0.0f, 60.0f));
	scene.UpdateTransforms();

	visibility->OnUpdate(Ember::Test::FixedStep(), scene.Ptr());

	EB_EXPECT_MSG(visibility->IsVisible(inView.GetEntityHandle()), "an entity in front of the camera was culled");
	EB_EXPECT_MSG(!visibility->IsVisible(behind.GetEntityHandle()), "an entity behind the camera was reported visible");

	visibility->OnSceneDetach(scene.Ptr());
}

EB_TEST_CASE(Render, VisibilityGraceWindowKeepsRecentlySeenEntities, Integration)
{
	// Hysteresis: an entity that just left the frustum stays "relevant" for a few frames so
	// expensive per-entity work isn't torn on and off as it flickers across the boundary.
	Ember::Test::RequireDefaultAssets();

	auto visibility = Sys<VisibilitySystem>();
	SceneFixture scene("VisibilityGraceScene");
	visibility->OnSceneDetach(scene.Ptr());
	visibility->SetGraceFrames(5);

	MakeActiveCamera(*scene, Vector3f(0.0f));
	Entity subject = MakeRenderableCube(*scene, "Subject", Vector3f(0.0f, 0.0f, -20.0f));
	scene.UpdateTransforms();

	visibility->OnUpdate(Ember::Test::FixedStep(), scene.Ptr());
	EB_CHECK_MSG(visibility->IsVisible(subject.GetEntityHandle()), "the subject was not visible to begin with");

	// Move it behind the camera and tick a couple of frames.
	subject.GetComponent<TransformComponent>().Position = Vector3f(0.0f, 0.0f, 60.0f);
	scene.UpdateTransforms();
	visibility->OnUpdate(Ember::Test::FixedStep(), scene.Ptr());
	visibility->OnUpdate(Ember::Test::FixedStep(), scene.Ptr());

	EB_EXPECT_MSG(!visibility->IsVisible(subject.GetEntityHandle()), "the entity is off screen but still reports visible");
	EB_EXPECT_MSG(visibility->IsRelevant(subject.GetEntityHandle()),
		"the grace window expired immediately - hysteresis is not working");

	// Past the grace window it drops out entirely.
	for (int i = 0; i < 8; ++i)
		visibility->OnUpdate(Ember::Test::FixedStep(), scene.Ptr());

	EB_EXPECT_MSG(!visibility->IsRelevant(subject.GetEntityHandle()),
		"the entity stayed relevant long after the grace window should have expired");

	visibility->SetGraceFrames(10); // restore the engine default
	visibility->OnSceneDetach(scene.Ptr());
}

EB_TEST_CASE(Render, DisablingVisibilityMakesEverythingVisible, Integration)
{
	Ember::Test::RequireDefaultAssets();

	auto visibility = Sys<VisibilitySystem>();
	SceneFixture scene("VisibilityDisabledScene");
	visibility->OnSceneDetach(scene.Ptr());

	MakeActiveCamera(*scene, Vector3f(0.0f));
	Entity behind = MakeRenderableCube(*scene, "Behind", Vector3f(0.0f, 0.0f, 60.0f));
	scene.UpdateTransforms();

	visibility->SetEnabled(false);
	visibility->OnUpdate(Ember::Test::FixedStep(), scene.Ptr());

	EB_EXPECT_MSG(visibility->IsVisible(behind.GetEntityHandle()),
		"with the visibility system disabled everything must report visible");

	visibility->SetEnabled(true); // restore the engine default
	visibility->OnSceneDetach(scene.Ptr());
}

//////////////////////////////////////////////////////////////////////////
// Render pipeline smoke tests
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Render, ViewportResizeIsIdempotentAndSurvivesRepeats, Integration)
{
	// Resizing reallocates every deferred render target and every post-process buffer. Doing it
	// repeatedly - and back to the original size - must leave the pipeline able to draw. This is the
	// exact path a stale framebuffer texture handle corrupts, and that class of bug only ever
	// surfaces in optimized builds, so run this test in Release as well as Debug.
	//
	// (Note this asserts on SURVIVAL, not on RenderSystem::GetViewportSize(): that getter reports the
	// active camera's viewport, which is set when a scene renders, not by OnViewportResize.)
	Ember::Test::RequireDefaultAssets();

	auto renderSystem = Sys<RenderSystem>();
	const Window& window = Application::Instance().GetWindow();
	const uint32_t width = window.GetWidth();
	const uint32_t height = window.GetHeight();

	SceneFixture scene("ResizeScene");
	MakeRenderableCube(*scene, "Cube", Vector3f(0.0f, 0.0f, -5.0f));
	scene.UpdateTransforms();

	const uint32_t sizes[][2] = { { width, height }, { 640, 360 }, { 1, 1 }, { 1920, 1080 }, { width, height } };
	for (const auto& size : sizes)
	{
		renderSystem->OnViewportResize(size[0], size[1]);
		RenderAction::SetViewport(0, 0, size[0], size[1]);
		scene.TickEdit();
	}

	// Restore the window-sized targets for whatever test runs next, and confirm one more frame
	// still goes through cleanly.
	renderSystem->OnViewportResize(width, height);
	RenderAction::SetViewport(0, 0, width, height);
	scene.TickEdit();

	EB_EXPECT_MSG(scene->GetAllEntities().size() == 1, "the scene did not survive the resize churn");
}

EB_TEST_CASE(Render, EntityIdPassIdentifiesThePixelUnderTheCursor, Integration)
{
	// The editor's click-to-select reads an entity ID out of the G-buffer. It is easy to break
	// silently (a changed attachment index, a buffer that stops being written) because nothing
	// else in the engine reads that attachment.
	Ember::Test::RequireDefaultAssets();

	auto renderSystem = Sys<RenderSystem>();
	const Window& window = Application::Instance().GetWindow();
	const uint32_t width = window.GetWidth();
	const uint32_t height = window.GetHeight();
	renderSystem->OnViewportResize(width, height);

	SceneFixture scene("PickingScene");
	// The light is created FIRST on purpose, so the cube does not end up as entity handle 0 -
	// that would make "the centre pixel is the cube" indistinguishable from a zero-filled buffer.
	Entity sun = MakeEntityAt(*scene, "Sun", Vector3f(0.0f, 10.0f, 0.0f),
		Vector3f(Math::Radians(-50.0f), Math::Radians(-30.0f), 0.0f));
	sun.AttachComponent<DirectionalLightComponent>();
	Entity cube = MakeRenderableCube(*scene, "PickTarget", Vector3f(0.0f, 0.0f, 0.0f));
	scene.UpdateTransforms();
	EB_CHECK_MSG(cube.GetEntityHandle() != 0, "test setup error: the pick target must not be entity 0");

	// A camera looking straight at the cube from +Z, so the cube covers the centre of the frame.
	Camera camera;
	camera.SetPerspective(60.0f, 0.1f, 100.0f);
	camera.SetViewportSize(width, height);

	RenderPassSettings settings;
	settings.ActiveCamera = &camera;
	settings.CameraTransform = Math::Inverse(
		Math::LookAt(Vector3f(0.0f, 0.0f, 5.0f), Vector3f(0.0f), Vector3f(0.0f, 1.0f, 0.0f)));
	settings.DrawHUD = false;

	RenderAction::SetViewport(0, 0, width, height);
	// Two passes so any first-frame state has settled before the readback.
	for (int i = 0; i < 2; ++i)
		scene->OnUpdateEdit(Ember::Test::FixedStep(), settings);

	const EntityID picked = renderSystem->GetEntityIDAtPixel(width / 2, height / 2);
	EB_EXPECT_MSG(picked == cube.GetEntityHandle(),
		"the centre pixel reported entity " + std::to_string(picked)
		+ " but the cube is entity " + std::to_string(cube.GetEntityHandle()));

	// A corner pixel shows empty space and must report the invalid sentinel, not entity 0.
	const EntityID empty = renderSystem->GetEntityIDAtPixel(2, 2);
	EB_EXPECT_MSG(empty != cube.GetEntityHandle(), "a pixel of empty space resolved to the cube");
}
