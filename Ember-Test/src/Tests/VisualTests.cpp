// Three levels of strictness: frame sanity (something was drawn) and determinism (the same scene twice
// gives the same pixels) need no reference image and are machine-independent; golden images are the
// strictest but GPU-dependent. Mint goldens with EMBER_TEST_WRITE_GOLDEN=1; a missing one skips.

#include <Ember.h>

#include "TestFramework.h"
#include "TestHelpers.h"

#include <cstdlib>
#include <filesystem>
#include <string>

using namespace Ember;
using Ember::Test::Type::Visual;
using Ember::Test::MakeEntityAt;
using Ember::Test::SceneFixture;
using Ember::Test::Sys;

namespace {

	const std::string kGoldenDir = "Ember-Test/golden";

	bool ShouldWriteGoldens()
	{
		return std::getenv("EMBER_TEST_WRITE_GOLDEN") != nullptr;
	}

	struct Viewport { uint32_t Width = 0; uint32_t Height = 0; };

	Viewport WindowViewport()
	{
		const Window& window = Application::Instance().GetWindow();
		return Viewport{ window.GetWidth(), window.GetHeight() };
	}

	// Renders `scene` to the back buffer from a fixed camera. Deterministic by construction: no
	// wall-clock time, no input, a fixed timestep. Two passes so any first-frame state (freshly
	// allocated render targets, lazily uploaded uniforms) has settled before the readback.
	void RenderSceneToBackbuffer(Scene& scene, const Vector3f& eye, const Vector3f& target, int passes = 2)
	{
		const Viewport viewport = WindowViewport();

		// Sizing the deferred targets to the window also drives the exact resize path a stale
		// framebuffer handle would corrupt.
		Sys<RenderSystem>()->OnViewportResize(viewport.Width, viewport.Height);

		// These projection parameters are part of the golden images' identity - changing the FOV or
		// either clip plane changes every pixel and invalidates every committed reference.
		Camera camera;
		camera.SetPerspective(60.0f, 0.1f, 100.0f);
		camera.SetViewportSize(viewport.Width, viewport.Height);

		RenderPassSettings settings;
		settings.ActiveCamera = &camera;
		settings.CameraTransform = Math::Inverse(Math::LookAt(eye, target, Vector3f(0.0f, 1.0f, 0.0f)));
		settings.DrawHUD = false;

		RenderAction::SetViewport(0, 0, viewport.Width, viewport.Height);
		for (int i = 0; i < passes; ++i)
			scene.OnUpdateEdit(Ember::Test::FixedStep(), settings);
	}

	// Compares the current back buffer to a golden, minting it instead when EMBER_TEST_WRITE_GOLDEN
	// is set and skipping when the golden does not exist yet.
	void CompareToGolden(const std::string& goldenName, int channelTolerance = 16, double maxDiffFraction = 0.02)
	{
		const Viewport viewport = WindowViewport();
		const std::string goldenPath = kGoldenDir + "/" + goldenName;

		if (ShouldWriteGoldens())
		{
			std::error_code ec;
			std::filesystem::create_directories(kGoldenDir, ec);
			const bool wrote = RenderSystem::CaptureBackbufferToPNG(viewport.Width, viewport.Height, goldenPath);
			EB_CHECK_MSG(wrote, "failed to write golden image to " + goldenPath);
			EB_NOTE("wrote golden image: " + goldenPath);
			return;
		}

		if (!std::filesystem::exists(goldenPath))
		{
			EB_SKIP("no golden image at '" + goldenPath
				+ "' yet - mint one from a known-good build with EMBER_TEST_WRITE_GOLDEN=1");
		}

		const bool matches = RenderSystem::CompareBackbufferToReference(
			viewport.Width, viewport.Height, goldenPath, channelTolerance, maxDiffFraction);

		EB_EXPECT_MSG(matches,
			"render differs from golden '" + goldenPath + "'. If this is an intended visual change, "
			"regenerate with EMBER_TEST_WRITE_GOLDEN=1; otherwise it is a regression.");
	}

	// Asserts the frame looks like a rendered image rather than a failure mode.
	void ExpectFrameLooksRendered(const std::string& label)
	{
		const Viewport viewport = WindowViewport();
		const RenderSystem::BackbufferStats stats =
			RenderSystem::ComputeBackbufferStats(viewport.Width, viewport.Height);

		EB_CHECK_MSG(stats.Valid, label + ": could not read the back buffer");

		EB_NOTE(label + ": mean luminance " + std::to_string(stats.MeanLuminance)
			+ ", stddev " + std::to_string(stats.StdDevLuminance)
			+ ", black " + std::to_string(stats.BlackFraction * 100.0) + "%"
			+ ", white " + std::to_string(stats.WhiteFraction * 100.0) + "%");

		// Nothing rendered at all - the single most common catastrophic failure.
		EB_EXPECT_MSG(stats.BlackFraction < 0.98,
			label + ": the frame is almost entirely black (" + std::to_string(stats.BlackFraction * 100.0)
			+ "% pure black) - nothing was drawn");

		// Blown out - runaway exposure, a bad IBL intensity, an uninitialised uniform.
		EB_EXPECT_MSG(stats.WhiteFraction < 0.90,
			label + ": the frame is almost entirely white (" + std::to_string(stats.WhiteFraction * 100.0)
			+ "% saturated) - lighting or tone mapping has blown out");

		// A flat fill of any single colour has zero variation: geometry never made it through.
		EB_EXPECT_MSG(stats.StdDevLuminance > 0.005,
			label + ": the frame has almost no tonal variation (stddev "
			+ std::to_string(stats.StdDevLuminance) + ") - it looks like a flat clear colour");
	}

	// A small, fully deterministic scene: a flat ground, a rotated cube and a directional light,
	// using the engine's built-in cube mesh and standard geometry material. Exercises the whole
	// deferred path (geometry -> lighting -> IBL -> shadows).
	void BuildDefaultScene(Scene& scene)
	{
		Entity ground = MakeEntityAt(scene, "Ground", Vector3f(0.0f, -0.5f, 0.0f), Vector3f(0.0f), Vector3f(20.0f, 0.5f, 20.0f));
		ground.AttachComponent<StaticMeshComponent>(UUID(Constants::Assets::CubeMeshUUID));
		ground.AttachComponent<MaterialComponent>(UUID(Constants::Assets::StandardGeometryMatUUID));

		Entity cube = MakeEntityAt(scene, "Cube", Vector3f(0.0f, 0.5f, 0.0f), Vector3f(0.0f, Math::Radians(30.0f), 0.0f));
		cube.AttachComponent<StaticMeshComponent>(UUID(Constants::Assets::CubeMeshUUID));
		cube.AttachComponent<MaterialComponent>(UUID(Constants::Assets::StandardGeometryMatUUID));

		Entity light = MakeEntityAt(scene, "Sun", Vector3f(0.0f, 10.0f, 0.0f),
			Vector3f(Math::Radians(-50.0f), Math::Radians(-30.0f), 0.0f));
		light.AttachComponent<DirectionalLightComponent>();
	}

	// A punctual-light scene: no directional light at all, so it isolates the point/spot light
	// paths and their attenuation from the sun-plus-IBL path the default scene covers.
	void BuildPointLightScene(Scene& scene)
	{
		Entity ground = MakeEntityAt(scene, "Ground", Vector3f(0.0f, -0.5f, 0.0f), Vector3f(0.0f), Vector3f(20.0f, 0.5f, 20.0f));
		ground.AttachComponent<StaticMeshComponent>(UUID(Constants::Assets::CubeMeshUUID));
		ground.AttachComponent<MaterialComponent>(UUID(Constants::Assets::StandardGeometryMatUUID));

		// Three spheres in a row, each lit by its own coloured point light.
		const Vector3f colours[3] = {
			Vector3f(1.0f, 0.2f, 0.2f),
			Vector3f(0.2f, 1.0f, 0.2f),
			Vector3f(0.2f, 0.2f, 1.0f)
		};

		for (int i = 0; i < 3; ++i)
		{
			const float x = (float)(i - 1) * 3.0f;

			Entity sphere = MakeEntityAt(scene, "Sphere" + std::to_string(i), Vector3f(x, 0.5f, 0.0f));
			sphere.AttachComponent<StaticMeshComponent>(UUID(Constants::Assets::SphereMeshUUID));
			sphere.AttachComponent<MaterialComponent>(UUID(Constants::Assets::StandardGeometryMatUUID));

			Entity light = MakeEntityAt(scene, "PointLight" + std::to_string(i), Vector3f(x, 2.5f, 1.0f));
			light.AttachComponent<PointLightComponent>(colours[i], 40.0f, 10.0f);
		}
	}

} // namespace

//////////////////////////////////////////////////////////////////////////
// Frame sanity (no reference image required)
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Render, DefaultSceneProducesAPlausibleFrame, Visual)
{
	Ember::Test::RequireDefaultAssets();

	SceneFixture scene("VisualSanityScene");
	BuildDefaultScene(*scene);
	scene.UpdateTransforms();

	RenderSceneToBackbuffer(*scene, Vector3f(6.0f, 5.0f, 8.0f), Vector3f(0.0f, 0.5f, 0.0f));
	ExpectFrameLooksRendered("default scene");
}

EB_TEST_CASE(Render, PointLitSceneProducesAPlausibleFrame, Visual)
{
	Ember::Test::RequireDefaultAssets();

	SceneFixture scene("VisualPointLightSanityScene");
	BuildPointLightScene(*scene);
	scene.UpdateTransforms();

	RenderSceneToBackbuffer(*scene, Vector3f(0.0f, 4.0f, 9.0f), Vector3f(0.0f, 0.5f, 0.0f));
	ExpectFrameLooksRendered("point-lit scene");
}

EB_TEST_CASE(Render, EmptySceneIsDarkButDoesNotCrash, Visual)
{
	// The degenerate case: no geometry, no lights. It must render cleanly (a clear colour or the
	// skybox) rather than fall over or leave the pipeline in a state that breaks the next frame.
	SceneFixture scene("VisualEmptyScene");
	RenderSceneToBackbuffer(*scene, Vector3f(0.0f, 2.0f, 5.0f), Vector3f(0.0f));

	const Viewport viewport = WindowViewport();
	const RenderSystem::BackbufferStats stats =
		RenderSystem::ComputeBackbufferStats(viewport.Width, viewport.Height);
	EB_CHECK(stats.Valid);
	EB_NOTE("empty scene mean luminance: " + std::to_string(stats.MeanLuminance));

	// Whatever it draws, it must not be a saturated white screen.
	EB_EXPECT_LT(stats.WhiteFraction, 0.5);
}

//////////////////////////////////////////////////////////////////////////
// Determinism (no reference image required, machine-independent)
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Render, RenderingTheSameSceneTwiceIsIdentical, Visual)
{
	// Both frames come from the same machine and build, so ANY difference is real - uninitialised
	// memory, an uncleared buffer, or state left behind by the previous pass.
	Ember::Test::RequireDefaultAssets();

	const Viewport viewport = WindowViewport();
	const std::string capturePath = Ember::Test::TempFile("determinism_frame.png");
	Ember::Test::RemoveTempFile(capturePath);

	const Vector3f eye(6.0f, 5.0f, 8.0f);
	const Vector3f target(0.0f, 0.5f, 0.0f);

	{
		SceneFixture first("VisualDeterminismSceneA");
		BuildDefaultScene(*first);
		first.UpdateTransforms();
		RenderSceneToBackbuffer(*first, eye, target);

		EB_CHECK_MSG(RenderSystem::CaptureBackbufferToPNG(viewport.Width, viewport.Height, capturePath),
			"failed to capture the first frame to " + capturePath);
	}

	{
		SceneFixture second("VisualDeterminismSceneB");
		BuildDefaultScene(*second);
		second.UpdateTransforms();
		RenderSceneToBackbuffer(*second, eye, target);

		// Tolerances are tight on purpose: same machine, same build, same scene. A handful of
		// pixels of slack absorbs nothing more than PNG round-tripping.
		const bool identical = RenderSystem::CompareBackbufferToReference(
			viewport.Width, viewport.Height, capturePath, /*channelTolerance*/ 2, /*maxDiffFraction*/ 0.001);

		EB_EXPECT_MSG(identical,
			"the same scene rendered twice produced different pixels - something in the frame is "
			"reading uninitialised or leftover state");
	}

	Ember::Test::RemoveTempFile(capturePath);
}

//////////////////////////////////////////////////////////////////////////
// Golden images
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Render, DefaultSceneGolden, Visual)
{
	Ember::Test::RequireDefaultAssets();

	SceneFixture scene("VisualTestScene");
	BuildDefaultScene(*scene);
	scene.UpdateTransforms();

	RenderSceneToBackbuffer(*scene, Vector3f(6.0f, 5.0f, 8.0f), Vector3f(0.0f, 0.5f, 0.0f));

	// Tolerances catch gross corruption (blowout / channel shift / black frame) while absorbing the
	// small floating-point differences between Debug and Release. Tighten once the pipeline settles.
	CompareToGolden("default_scene.png", /*channelTolerance*/ 16, /*maxDiffFraction*/ 0.02);
}

EB_TEST_CASE(Render, PointLightSceneGolden, Visual)
{
	Ember::Test::RequireDefaultAssets();

	SceneFixture scene("VisualPointLightScene");
	BuildPointLightScene(*scene);
	scene.UpdateTransforms();

	RenderSceneToBackbuffer(*scene, Vector3f(0.0f, 4.0f, 9.0f), Vector3f(0.0f, 0.5f, 0.0f));
	CompareToGolden("point_light_scene.png", /*channelTolerance*/ 16, /*maxDiffFraction*/ 0.02);
}

EB_TEST_CASE(Render, ShadowCastingSceneGolden, Visual)
{
	// A raking light and a tall block, so shadows dominate the frame. Shadow regressions barely move
	// the average colour, so they need a scene built around them to be detectable at all.
	Ember::Test::RequireDefaultAssets();

	SceneFixture scene("VisualShadowScene");

	Entity ground = MakeEntityAt(*scene, "Ground", Vector3f(0.0f, -0.5f, 0.0f), Vector3f(0.0f), Vector3f(30.0f, 0.5f, 30.0f));
	ground.AttachComponent<StaticMeshComponent>(UUID(Constants::Assets::CubeMeshUUID));
	ground.AttachComponent<MaterialComponent>(UUID(Constants::Assets::StandardGeometryMatUUID));

	Entity pillar = MakeEntityAt(*scene, "Pillar", Vector3f(-1.5f, 2.0f, 0.0f), Vector3f(0.0f), Vector3f(0.6f, 4.0f, 0.6f));
	pillar.AttachComponent<StaticMeshComponent>(UUID(Constants::Assets::CubeMeshUUID));
	pillar.AttachComponent<MaterialComponent>(UUID(Constants::Assets::StandardGeometryMatUUID));

	Entity block = MakeEntityAt(*scene, "Block", Vector3f(2.0f, 0.5f, 0.0f));
	block.AttachComponent<StaticMeshComponent>(UUID(Constants::Assets::CubeMeshUUID));
	block.AttachComponent<MaterialComponent>(UUID(Constants::Assets::StandardGeometryMatUUID));

	Entity sun = MakeEntityAt(*scene, "Sun", Vector3f(0.0f, 10.0f, 0.0f),
		Vector3f(Math::Radians(-25.0f), Math::Radians(-70.0f), 0.0f));
	sun.AttachComponent<DirectionalLightComponent>();

	scene.UpdateTransforms();

	RenderSceneToBackbuffer(*scene, Vector3f(7.0f, 4.0f, 9.0f), Vector3f(0.0f, 1.0f, 0.0f));
	ExpectFrameLooksRendered("shadow scene");
	CompareToGolden("shadow_scene.png", /*channelTolerance*/ 20, /*maxDiffFraction*/ 0.03);
}
