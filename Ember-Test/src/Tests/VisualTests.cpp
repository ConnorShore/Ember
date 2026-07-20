// VISUAL / GOLDEN-IMAGE TESTS
// ---------------------------
// Render a known scene and compare the pixels to a committed reference ("golden") image. This is the
// class of test that would have caught the stale-G-buffer-handle bug immediately in Release: it asserts
// on the actual rendered output regardless of the root cause. Run in Debug AND Release in CI.
//
// Workflow:
//   1) Mint/refresh the reference once in a known-good build:  set EMBER_TEST_WRITE_GOLDEN=1 and run.
//   2) Commit Ember-Test/golden/*.png.
//   3) Normal runs compare against it; a mismatch fails the test with the diff percentage.

#include <Ember.h>
#include "TestFramework.h"

#include <cstdlib>
#include <filesystem>
#include <string>

using namespace Ember;
using Ember::Test::Type::Visual;

namespace {

	// A small, fully deterministic scene: a flat ground + a rotated cube + a directional light, using the
	// engine's built-in cube mesh and standard geometry material. Exercises the full deferred path
	// (geometry -> lighting -> IBL -> shadows) — exactly where the handle bug lived.
	void BuildTestScene(Scene& scene)
	{
		Entity ground = scene.AddEntity("Ground");
		ground.AttachComponent<TransformComponent>(Vector3f(0.0f, -0.5f, 0.0f), Vector3f(0.0f), Vector3f(20.0f, 0.5f, 20.0f));
		ground.AttachComponent<StaticMeshComponent>(UUID(Constants::Assets::CubeMeshUUID));
		ground.AttachComponent<MaterialComponent>(UUID(Constants::Assets::StandardGeometryMatUUID));

		Entity cube = scene.AddEntity("Cube");
		cube.AttachComponent<TransformComponent>(Vector3f(0.0f, 0.5f, 0.0f), Vector3f(0.0f, Math::Radians(30.0f), 0.0f), Vector3f(1.0f));
		cube.AttachComponent<StaticMeshComponent>(UUID(Constants::Assets::CubeMeshUUID));
		cube.AttachComponent<MaterialComponent>(UUID(Constants::Assets::StandardGeometryMatUUID));

		Entity light = scene.AddEntity("Sun");
		light.AttachComponent<TransformComponent>(Vector3f(0.0f, 10.0f, 0.0f), Vector3f(Math::Radians(-50.0f), Math::Radians(-30.0f), 0.0f), Vector3f(1.0f));
		light.AttachComponent<DirectionalLightComponent>();
	}

} // namespace

EB_TEST_CASE(Render, DefaultSceneGolden, Visual)
{
	auto& app = Application::Instance();
	const uint32_t w = app.GetWindow().GetWidth();
	const uint32_t h = app.GetWindow().GetHeight();

	// Size the deferred render targets to the window (this also drives the exact resize path that the
	// stale-handle bug lived in).
	app.GetSystem<RenderSystem>()->OnViewportResize(w, h);

	Scene scene("VisualTestScene", "");
	BuildTestScene(scene);

	// Fixed camera looking at the scene from an angle — deterministic, no dependence on wall-clock time.
	Camera camera;
	camera.SetPerspective(60.0f, 0.1f, 100.0f);
	camera.SetViewportSize(w, h);

	const Vector3f eye(6.0f, 5.0f, 8.0f);
	const Vector3f center(0.0f, 0.5f, 0.0f);

	RenderPassSettings settings;
	settings.ActiveCamera = &camera;
	settings.CameraTransform = Math::Inverse(Math::LookAt(eye, center, Vector3f(0.0f, 1.0f, 0.0f)));
	settings.DrawHUD = false;

	// Render the scene to the default framebuffer (back buffer). OnUpdateEdit runs the TransformSystem
	// then the RenderSystem with our camera. Two passes let any first-frame state settle before readback.
	RenderAction::SetViewport(0, 0, w, h);
	for (int i = 0; i < 2; ++i)
		scene.OnUpdateEdit(TimeStep(1.0f / 60.0f), settings);

	const std::string goldenPath = "Ember-Test/golden/default_scene.png";

	if (std::getenv("EMBER_TEST_WRITE_GOLDEN"))
	{
		std::filesystem::create_directories("Ember-Test/golden");
		const bool wrote = RenderSystem::CaptureBackbufferToPNG(w, h, goldenPath);
		EB_CHECK_MSG(wrote, "failed to write golden image to " + goldenPath);
		std::printf("         (wrote golden image: %s)\n", goldenPath.c_str());
		return;
	}

	// Tolerances catch gross corruption (blowout / channel-shift / black) while absorbing tiny FP
	// differences between Debug and Release. Tighten once you trust the pipeline.
	const bool matches = RenderSystem::CompareBackbufferToReference(w, h, goldenPath,
		/*channelTolerance*/ 16, /*maxDiffFraction*/ 0.02);
	EB_CHECK_MSG(matches,
		"render differs from golden '" + goldenPath + "'. If this is an intended visual change, "
		"regenerate with EMBER_TEST_WRITE_GOLDEN=1; otherwise it's a regression.");
}
