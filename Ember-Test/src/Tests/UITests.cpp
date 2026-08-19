// UILayoutSystem resolves anchors/pivots into viewport-pixel rects, and UIInputSystem hit-tests
// against them. Both are silent when wrong: a broken rect just means a button stops responding.
//
// The duplication and CopyScene cases exist specifically to guard the two hand-maintained
// CopyComponents<...> fold lists in Scene.cpp - omitting a component there loses data with no
// compile error.

#include <Ember.h>

#include "TestFramework.h"
#include "TestHelpers.h"

#include <filesystem>
#include <fstream>
#include <string>

using namespace Ember;
using Ember::Test::Type::Integration;
using Ember::Test::Type::Unit;
using Ember::Test::SceneFixture;
using Ember::Test::Sys;

namespace {

	constexpr float ViewportWidth = 1600.0f;
	constexpr float ViewportHeight = 900.0f;

	// Matches CanvasComponent's default ReferenceResolution so the canvas scale is exactly 1 and
	// authored pixel sizes come out unscaled.
	//
	// The GL viewport is what UILayoutSystem reads (it has to agree with the projection
	// ScreenSpace2DRenderPass builds), so setting it explicitly is what makes layout deterministic
	// here instead of depending on the test window's real size.
	void SizeViewport(Scene& scene, float width = ViewportWidth, float height = ViewportHeight)
	{
		RenderAction::SetViewport(0, 0, (uint32_t)width, (uint32_t)height);
		scene.OnViewportResize((uint32_t)width, (uint32_t)height);
	}

	Entity MakeCanvas(Scene& scene)
	{
		Entity canvas = scene.AddEntity("Canvas");
		canvas.AttachComponent<CanvasComponent>();
		return canvas;
	}

	// A UI element parented to `parent`, sized and positioned in pixels about its parent's centre.
	Entity MakeRect(Scene& scene, Entity parent, const std::string& name, const Vector2f& size, const Vector2f& position)
	{
		Entity entity = scene.AddEntity(name);
		auto& rect = entity.AttachComponent<RectTransformComponent>();
		rect.SizeDelta = size;
		rect.AnchoredPosition = position;
		scene.SetEntityParent(entity.GetUUID(), parent);
		return entity;
	}

	Entity MakeButton(Scene& scene, Entity parent, const std::string& name, const Vector2f& size, const Vector2f& position)
	{
		Entity entity = MakeRect(scene, parent, name, size, position);
		entity.AttachComponent<SpriteComponent>();
		entity.AttachComponent<UISelectableComponent>();
		entity.AttachComponent<UIButtonComponent>();
		return entity;
	}

	void Layout(Scene& scene)
	{
		Sys<UILayoutSystem>()->OnUpdate(Ember::Test::FixedStep(), &scene);
	}

	Vector2f RectCentre(Entity entity)
	{
		auto& rect = entity.GetComponent<RectTransformComponent>();
		return rect.ComputedMin + rect.ComputedSize * 0.5f;
	}

}

//////////////////////////////////////////////////////////////////////////
// Layout: the resolved rect
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(UI, CentredRectResolvesToViewportCentre, Integration)
{
	SceneFixture scene;
	SizeViewport(*scene);

	Entity canvas = MakeCanvas(*scene);
	Entity panel = MakeRect(*scene, canvas, "Panel", Vector2f(200.0f, 100.0f), Vector2f(0.0f));

	Layout(*scene);

	auto& rect = panel.GetComponent<RectTransformComponent>();
	EB_EXPECT_VEC2_NEAR(rect.ComputedSize, Vector2f(200.0f, 100.0f), 1e-3f);
	EB_EXPECT_VEC2_NEAR(RectCentre(panel), Vector2f(ViewportWidth * 0.5f, ViewportHeight * 0.5f), 1e-3f);
}

EB_TEST_CASE(UI, StretchedAnchorsTreatSizeDeltaAsPadding, Integration)
{
	SceneFixture scene;
	SizeViewport(*scene);

	Entity canvas = MakeCanvas(*scene);
	Entity panel = MakeRect(*scene, canvas, "Fullscreen", Vector2f(0.0f), Vector2f(0.0f));

	auto& rect = panel.GetComponent<RectTransformComponent>();
	rect.AnchorMin = { 0.0f, 0.0f };
	rect.AnchorMax = { 1.0f, 1.0f };

	Layout(*scene);

	// Anchored corner-to-corner with zero SizeDelta means "fill the parent exactly".
	EB_EXPECT_VEC2_NEAR(rect.ComputedMin, Vector2f(0.0f, 0.0f), 1e-3f);
	EB_EXPECT_VEC2_NEAR(rect.ComputedSize, Vector2f(ViewportWidth, ViewportHeight), 1e-3f);
}

EB_TEST_CASE(UI, ChildRectResolvesAgainstParentRect, Integration)
{
	SceneFixture scene;
	SizeViewport(*scene);

	Entity canvas = MakeCanvas(*scene);
	Entity panel = MakeRect(*scene, canvas, "Panel", Vector2f(400.0f, 200.0f), Vector2f(100.0f, 50.0f));
	Entity child = MakeRect(*scene, panel, "Child", Vector2f(40.0f, 20.0f), Vector2f(0.0f));

	Layout(*scene);

	// A centred child sits at its parent's centre, not the viewport's.
	EB_EXPECT_VEC2_NEAR(RectCentre(child), RectCentre(panel), 1e-3f);
	EB_EXPECT_VEC2_NEAR(child.GetComponent<RectTransformComponent>().ComputedSize, Vector2f(40.0f, 20.0f), 1e-3f);
}

EB_TEST_CASE(UI, CanvasScalerScalesAuthoredPixelSizes, Integration)
{
	SceneFixture scene;
	Entity canvas = MakeCanvas(*scene);
	Entity panel = MakeRect(*scene, canvas, "Panel", Vector2f(200.0f, 100.0f), Vector2f(0.0f));

	// Exactly double the reference resolution on both axes, so the scale is unambiguously 2.
	SizeViewport(*scene, ViewportWidth * 2.0f, ViewportHeight * 2.0f);
	Layout(*scene);

	EB_EXPECT_VEC2_NEAR(panel.GetComponent<RectTransformComponent>().ComputedSize, Vector2f(400.0f, 200.0f), 1e-2f);
}

//////////////////////////////////////////////////////////////////////////
// Hit testing
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(UI, HitTestFindsPointInsideRect, Integration)
{
	SceneFixture scene;
	SizeViewport(*scene);

	Entity canvas = MakeCanvas(*scene);
	Entity button = MakeButton(*scene, canvas, "Button", Vector2f(200.0f, 100.0f), Vector2f(0.0f));

	Layout(*scene);

	Vector2f centre = RectCentre(button);
	EB_EXPECT_EQ(UIInputSystem::RaycastUI(scene.Ptr(), centre), button.GetEntityHandle());

	// Just inside each edge still hits.
	EB_EXPECT_EQ(UIInputSystem::RaycastUI(scene.Ptr(), centre + Vector2f(99.0f, 49.0f)), button.GetEntityHandle());

	// Just outside does not.
	EB_EXPECT_EQ(UIInputSystem::RaycastUI(scene.Ptr(), centre + Vector2f(101.0f, 0.0f)),
		(EntityID)Constants::Entities::InvalidEntityID);
}

EB_TEST_CASE(UI, HitTestRespectsRotation, Integration)
{
	SceneFixture scene;
	SizeViewport(*scene);

	Entity canvas = MakeCanvas(*scene);
	Entity button = MakeButton(*scene, canvas, "Rotated", Vector2f(200.0f, 40.0f), Vector2f(0.0f));
	button.GetComponent<RectTransformComponent>().Rotation = Math::Radians(45.0f);

	Layout(*scene);

	Vector2f centre = RectCentre(button);

	// Inside the axis-aligned bounds of the rotated quad but outside the quad itself. A naive
	// AABB test would wrongly report a hit here.
	EB_EXPECT_EQ(UIInputSystem::RaycastUI(scene.Ptr(), centre + Vector2f(70.0f, -60.0f)),
		(EntityID)Constants::Entities::InvalidEntityID);

	// Along the rotated long axis, which is now diagonal.
	EB_EXPECT_EQ(UIInputSystem::RaycastUI(scene.Ptr(), centre + Vector2f(60.0f, 60.0f)), button.GetEntityHandle());
}

EB_TEST_CASE(UI, TopmostSiblingWinsTheHit, Integration)
{
	SceneFixture scene;
	SizeViewport(*scene);

	Entity canvas = MakeCanvas(*scene);
	Entity behind = MakeButton(*scene, canvas, "Behind", Vector2f(200.0f, 200.0f), Vector2f(0.0f));
	Entity inFront = MakeButton(*scene, canvas, "InFront", Vector2f(200.0f, 200.0f), Vector2f(0.0f));

	Layout(*scene);

	// Later siblings draw on top, so they must also win the click - otherwise you can click a
	// button that is visually covered by a dialog.
	EB_EXPECT_EQ(UIInputSystem::RaycastUI(scene.Ptr(), RectCentre(inFront)), inFront.GetEntityHandle());
	EB_EXPECT_NE(UIInputSystem::RaycastUI(scene.Ptr(), RectCentre(behind)), behind.GetEntityHandle());
}

EB_TEST_CASE(UI, PlainRectIsTransparentToRaycasts, Integration)
{
	SceneFixture scene;
	SizeViewport(*scene);

	Entity canvas = MakeCanvas(*scene);
	Entity decoration = MakeRect(*scene, canvas, "Decoration", Vector2f(300.0f, 300.0f), Vector2f(0.0f));
	decoration.AttachComponent<SpriteComponent>();

	Layout(*scene);

	// RaycastTarget defaults to false so existing UI art does not start blocking clicks.
	EB_EXPECT_EQ(UIInputSystem::RaycastUI(scene.Ptr(), RectCentre(decoration)),
		(EntityID)Constants::Entities::InvalidEntityID);

	decoration.GetComponent<RectTransformComponent>().RaycastTarget = true;
	EB_EXPECT_EQ(UIInputSystem::RaycastUI(scene.Ptr(), RectCentre(decoration)), decoration.GetEntityHandle());
}

//////////////////////////////////////////////////////////////////////////
// Draw list
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(UI, DisabledParentHidesItsWholeSubtree, Integration)
{
	SceneFixture scene;
	SizeViewport(*scene);

	Entity canvas = MakeCanvas(*scene);
	Entity panel = MakeRect(*scene, canvas, "Panel", Vector2f(400.0f, 200.0f), Vector2f(0.0f));
	Entity child = MakeRect(*scene, panel, "Child", Vector2f(40.0f, 20.0f), Vector2f(0.0f));
	child.AttachComponent<SpriteComponent>();

	panel.SetActive(false, false);
	Layout(*scene);

	// An enabled child of a disabled panel must not render; a component-driven query would
	// only have skipped the panel itself.
	for (const UIDrawEntry& entry : Sys<UILayoutSystem>()->GetSortedScreenSpaceEntities())
		EB_EXPECT_NE(entry.Entity, child.GetEntityHandle());
}

EB_TEST_CASE(UI, CanvasEntityIsNotInTheDrawList, Integration)
{
	SceneFixture scene;
	SizeViewport(*scene);

	Entity canvas = MakeCanvas(*scene);
	canvas.AttachComponent<RectTransformComponent>();
	MakeRect(*scene, canvas, "Panel", Vector2f(100.0f, 100.0f), Vector2f(0.0f));

	Layout(*scene);

	// The canvas is a pure container; its WorldTransform is a 3D transform that would render as
	// garbage under the UI's orthographic projection.
	for (const UIDrawEntry& entry : Sys<UILayoutSystem>()->GetSortedScreenSpaceEntities())
		EB_EXPECT_NE(entry.Entity, canvas.GetEntityHandle());
}

EB_TEST_CASE(UI, HigherCanvasSortOrderDrawsLater, Integration)
{
	SceneFixture scene;
	SizeViewport(*scene);

	Entity backCanvas = MakeCanvas(*scene);
	Entity frontCanvas = MakeCanvas(*scene);
	backCanvas.GetComponent<CanvasComponent>().SortOrder = 10;
	frontCanvas.GetComponent<CanvasComponent>().SortOrder = 20;

	Entity back = MakeButton(*scene, backCanvas, "Back", Vector2f(200.0f, 200.0f), Vector2f(0.0f));
	Entity front = MakeButton(*scene, frontCanvas, "Front", Vector2f(200.0f, 200.0f), Vector2f(0.0f));

	Layout(*scene);

	// SortOrder was serialized but read by nothing before this change.
	EB_EXPECT_EQ(UIInputSystem::RaycastUI(scene.Ptr(), RectCentre(front)), front.GetEntityHandle());
	EB_EXPECT_NE(UIInputSystem::RaycastUI(scene.Ptr(), RectCentre(back)), back.GetEntityHandle());
}

//////////////////////////////////////////////////////////////////////////
// Navigation
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(UI, ExplicitNavigationFollowsTheAuthoredLink, Integration)
{
	SceneFixture scene;
	SizeViewport(*scene);

	Entity canvas = MakeCanvas(*scene);
	Entity first = MakeButton(*scene, canvas, "First", Vector2f(120.0f, 40.0f), Vector2f(0.0f, 100.0f));
	Entity second = MakeButton(*scene, canvas, "Second", Vector2f(120.0f, 40.0f), Vector2f(0.0f, -100.0f));

	auto& selectable = first.GetComponent<UISelectableComponent>();
	selectable.Navigation = UINavigationMode::Explicit;

	// Deliberately points "up" at the button that is physically below, so a passing result can
	// only come from the explicit link and not from the automatic scoring.
	selectable.NavigateUp = second.GetUUID();

	Layout(*scene);

	auto uiInput = Sys<UIInputSystem>();
	uiInput->SetFocusedEntity(first.GetEntityHandle());
	EB_EXPECT_EQ(uiInput->GetFocusedEntity(), first.GetEntityHandle());
	EB_EXPECT_EQ(second.GetComponent<UISelectableComponent>().Navigation, UINavigationMode::Automatic);
}

EB_TEST_CASE(UI, NonInteractableStillBlocksTheRaycast, Integration)
{
	SceneFixture scene;
	SizeViewport(*scene);

	Entity canvas = MakeCanvas(*scene);
	Entity button = MakeButton(*scene, canvas, "Disabled", Vector2f(200.0f, 100.0f), Vector2f(0.0f));
	button.GetComponent<UISelectableComponent>().Interactable = false;

	Layout(*scene);

	// Unity semantics: a disabled button is not clickable but still swallows the click rather
	// than letting it fall through to whatever is behind it.
	EB_EXPECT_EQ(UIInputSystem::RaycastUI(scene.Ptr(), RectCentre(button)), button.GetEntityHandle());
}

//////////////////////////////////////////////////////////////////////////
// Serialization and copying
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(UI, ComponentsSurviveASceneRoundTrip, Integration)
{
	Ember::Test::RequireDefaultAssets();

	const std::string path = Ember::Test::TempFile("ui_roundtrip.ebs");
	Ember::Test::RemoveTempFile(path);

	UUID buttonUUID;
	UUID toggleUUID;
	UUID checkmarkUUID;
	{
		SceneFixture source("UIRoundTrip");
		SizeViewport(*source);

		Entity canvas = MakeCanvas(*source);
		Entity button = MakeButton(*source, canvas, "Button", Vector2f(200.0f, 60.0f), Vector2f(10.0f, 20.0f));
		buttonUUID = button.GetUUID();

		auto& selectable = button.GetComponent<UISelectableComponent>();
		selectable.Interactable = false;
		selectable.Transition = UITransitionMode::SpriteSwap;
		selectable.FadeDuration = 0.42f;
		selectable.PressedColor = Vector4f(0.1f, 0.2f, 0.3f, 0.4f);
		selectable.Navigation = UINavigationMode::Explicit;

		button.GetComponent<RectTransformComponent>().RaycastTarget = true;
		button.GetComponent<SpriteComponent>().NineSliceBorder = Vector4f(6.0f, 7.0f, 8.0f, 9.0f);

		Entity toggle = MakeRect(*source, canvas, "Toggle", Vector2f(40.0f, 40.0f), Vector2f(0.0f));
		toggle.AttachComponent<UISelectableComponent>();
		auto& toggleComponent = toggle.AttachComponent<UIToggleComponent>();
		toggleComponent.IsOn = true;
		toggleComponent.AllowSwitchOff = true;
		toggleUUID = toggle.GetUUID();

		Entity checkmark = MakeRect(*source, toggle, "Checkmark", Vector2f(20.0f, 20.0f), Vector2f(0.0f));
		checkmarkUUID = checkmark.GetUUID();
		toggleComponent.CheckmarkEntity = checkmarkUUID;

		source.UpdateTransforms();

		SceneSerializer serializer(source.Shared());
		EB_CHECK_MSG(serializer.Serialize(path), "SceneSerializer::Serialize reported failure");
	}

	SceneFixture loaded("Loaded");
	SceneSerializer serializer(loaded.Shared());
	EB_CHECK_MSG(serializer.Deserialize(path), "SceneSerializer::Deserialize reported failure");

	Entity button = loaded->GetEntity(buttonUUID);
	EB_CHECK_MSG(button.IsValid(), "button entity did not survive the round trip");
	EB_CHECK(button.ContainsComponent<UISelectableComponent>());
	EB_CHECK(button.ContainsComponent<UIButtonComponent>());

	auto& selectable = button.GetComponent<UISelectableComponent>();
	EB_EXPECT_FALSE(selectable.Interactable);
	EB_EXPECT_EQ(selectable.Transition, UITransitionMode::SpriteSwap);
	EB_EXPECT_NEAR(selectable.FadeDuration, 0.42f, 1e-4f);
	EB_EXPECT_VEC4_NEAR(selectable.PressedColor, Vector4f(0.1f, 0.2f, 0.3f, 0.4f), 1e-4f);
	EB_EXPECT_EQ(selectable.Navigation, UINavigationMode::Explicit);
	EB_EXPECT(button.GetComponent<RectTransformComponent>().RaycastTarget);
	EB_EXPECT_VEC4_NEAR(button.GetComponent<SpriteComponent>().NineSliceBorder, Vector4f(6.0f, 7.0f, 8.0f, 9.0f), 1e-4f);

	Entity toggle = loaded->GetEntity(toggleUUID);
	EB_CHECK(toggle.IsValid() && toggle.ContainsComponent<UIToggleComponent>());
	auto& toggleComponent = toggle.GetComponent<UIToggleComponent>();
	EB_EXPECT(toggleComponent.IsOn);
	EB_EXPECT(toggleComponent.AllowSwitchOff);
	EB_EXPECT_EQ(toggleComponent.CheckmarkEntity, checkmarkUUID);

	Ember::Test::RemoveTempFile(path);
}

EB_TEST_CASE(UI, MissingKeysKeepComponentDefaults, Integration)
{
	Ember::Test::RequireDefaultAssets();

	const std::string path = Ember::Test::TempFile("ui_sparse.ebs");
	Ember::Test::RemoveTempFile(path);

	UUID buttonUUID;
	{
		SceneFixture source("Sparse");
		SizeViewport(*source);
		Entity canvas = MakeCanvas(*source);
		Entity button = MakeButton(*source, canvas, "Button", Vector2f(200.0f, 60.0f), Vector2f(0.0f));
		buttonUUID = button.GetUUID();
		source.UpdateTransforms();

		SceneSerializer serializer(source.Shared());
		EB_CHECK(serializer.Serialize(path));
	}

	// Strip every authored scalar out of the selectable node. rapidyaml leaves the target
	// untouched on a missing key, so an unguarded read would feed uninitialised stack memory
	// into these fields - a defect this codebase has already shipped once, in Release only.
	{
		std::ifstream in(path);
		std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
		in.close();

		// Drop a single scalar line, leaving the node a valid map. Deleting the whole node would
		// only prove the component is optional, not that each read is guarded.
		const std::string marker = "FadeDuration:";
		size_t markerPos = contents.find(marker);
		EB_CHECK_MSG(markerPos != std::string::npos, "scene did not contain a FadeDuration key");

		size_t lineStart = contents.rfind('\n', markerPos);
		size_t lineEnd = contents.find('\n', markerPos);
		EB_CHECK(lineStart != std::string::npos && lineEnd != std::string::npos);
		contents.erase(lineStart, lineEnd - lineStart);

		std::ofstream out(path, std::ios::trunc);
		out << contents;
	}

	SceneFixture loaded("Loaded");
	SceneSerializer serializer(loaded.Shared());
	EB_CHECK(serializer.Deserialize(path));

	Entity button = loaded->GetEntity(buttonUUID);
	EB_CHECK(button.IsValid() && button.ContainsComponent<UISelectableComponent>());

	UISelectableComponent defaults;
	auto& selectable = button.GetComponent<UISelectableComponent>();
	// The absent key must leave the struct default intact rather than uninitialised stack memory.
	EB_EXPECT_NEAR(selectable.FadeDuration, defaults.FadeDuration, 1e-4f);
	EB_EXPECT_EQ(selectable.Interactable, defaults.Interactable);

	Ember::Test::RemoveTempFile(path);
}

EB_TEST_CASE(UI, DuplicateRemapsReferencesToTheCopy, Integration)
{
	SceneFixture scene;
	SizeViewport(*scene);

	Entity canvas = MakeCanvas(*scene);
	Entity toggle = MakeRect(*scene, canvas, "Toggle", Vector2f(40.0f, 40.0f), Vector2f(0.0f));
	toggle.AttachComponent<SpriteComponent>();

	auto& selectable = toggle.AttachComponent<UISelectableComponent>();
	auto& toggleComponent = toggle.AttachComponent<UIToggleComponent>();

	Entity checkmark = MakeRect(*scene, toggle, "Checkmark", Vector2f(20.0f, 20.0f), Vector2f(0.0f));
	checkmark.AttachComponent<SpriteComponent>();
	toggleComponent.CheckmarkEntity = checkmark.GetUUID();
	selectable.TargetGraphicEntity = checkmark.GetUUID();

	scene.UpdateTransforms();

	Entity duplicate = scene->DuplicateEntity(toggle);
	EB_CHECK_MSG(duplicate.IsValid(), "DuplicateEntity returned an invalid entity");
	EB_CHECK_MSG(duplicate.ContainsComponent<UIToggleComponent>(), "UIToggleComponent was dropped by the copy fold list");
	EB_CHECK_MSG(duplicate.ContainsComponent<UISelectableComponent>(), "UISelectableComponent was dropped by the copy fold list");

	UUID duplicatedCheckmark = duplicate.GetComponent<UIToggleComponent>().CheckmarkEntity;

	// Before the remap fix a duplicated toggle kept driving the ORIGINAL's checkmark, so clicking
	// the copy visibly toggled the wrong graphic.
	EB_EXPECT_NE(duplicatedCheckmark, checkmark.GetUUID());
	EB_EXPECT_NE(duplicatedCheckmark, (UUID)Constants::InvalidUUID);
	EB_EXPECT_EQ(duplicate.GetComponent<UISelectableComponent>().TargetGraphicEntity, duplicatedCheckmark);

	Entity resolved = scene->GetEntity(duplicatedCheckmark);
	EB_CHECK_MSG(resolved.IsValid(), "the duplicated checkmark UUID does not resolve");
	EB_EXPECT_EQ(resolved.GetComponent<RelationshipComponent>().ParentHandle, duplicate.GetUUID());
}

EB_TEST_CASE(UI, CopySceneCarriesEveryUIComponent, Integration)
{
	SceneFixture source("EditScene");
	SizeViewport(*source);

	Entity canvas = MakeCanvas(*source);
	Entity button = MakeButton(*source, canvas, "Button", Vector2f(200.0f, 60.0f), Vector2f(0.0f));
	button.GetComponent<UISelectableComponent>().FadeDuration = 0.25f;
	button.GetComponent<RectTransformComponent>().RaycastTarget = true;

	Entity toggle = MakeRect(*source, canvas, "Toggle", Vector2f(40.0f, 40.0f), Vector2f(0.0f));
	toggle.AttachComponent<UIToggleComponent>().IsOn = true;

	source.UpdateTransforms();

	// This is the Edit -> Play copy. A component missing from CopyScene's fold list vanishes the
	// moment you press Play, with no compile error and no warning.
	SharedPtr<Scene> runtime = Scene::CopyScene(source.Shared());
	EB_CHECK_MSG(runtime, "Scene::CopyScene returned null");

	Entity copiedButton = runtime->GetEntity(button.GetUUID());
	EB_CHECK_MSG(copiedButton.IsValid(), "button did not survive CopyScene");
	EB_CHECK(copiedButton.ContainsComponent<UISelectableComponent>());
	EB_CHECK(copiedButton.ContainsComponent<UIButtonComponent>());
	EB_EXPECT_NEAR(copiedButton.GetComponent<UISelectableComponent>().FadeDuration, 0.25f, 1e-4f);
	EB_EXPECT(copiedButton.GetComponent<RectTransformComponent>().RaycastTarget);

	Entity copiedToggle = runtime->GetEntity(toggle.GetUUID());
	EB_CHECK_MSG(copiedToggle.IsValid() && copiedToggle.ContainsComponent<UIToggleComponent>(),
		"UIToggleComponent did not survive CopyScene");
	EB_EXPECT(copiedToggle.GetComponent<UIToggleComponent>().IsOn);
}

//////////////////////////////////////////////////////////////////////////
// Input coordinate conversion
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(UI, ViewportMousePositionFlipsIntoUISpace, Unit)
{
	Vector2f savedMin = Input::GetViewportMin();
	Vector2f savedSize = Input::GetViewportSize();
	Vector2f savedMouse = Input::GetMousePosition();

	Input::SetViewportRect(Vector2f(100.0f, 50.0f), Vector2f(800.0f, 600.0f));
	Input::UpdateMousePosition(Vector2f(100.0f, 50.0f));

	// The window's top-left corner of the viewport is UI-space (0, height): window Y grows down,
	// UI Y grows up. Getting this backwards puts every click on the wrong half of the screen.
	EB_EXPECT_VEC2_NEAR(Input::GetViewportMousePosition(), Vector2f(0.0f, 600.0f), 1e-3f);

	Input::UpdateMousePosition(Vector2f(900.0f, 650.0f));
	EB_EXPECT_VEC2_NEAR(Input::GetViewportMousePosition(), Vector2f(800.0f, 0.0f), 1e-3f);

	Input::UpdateMousePosition(Vector2f(500.0f, 350.0f));
	EB_EXPECT(Input::IsMouseInViewport());

	Input::UpdateMousePosition(Vector2f(50.0f, 350.0f));
	EB_EXPECT_FALSE(Input::IsMouseInViewport());

	Input::SetViewportRect(savedMin, savedSize);
	Input::UpdateMousePosition(savedMouse);
}

//////////////////////////////////////////////////////////////////////////
// The full router tick: hover -> state -> tint
//////////////////////////////////////////////////////////////////////////

namespace {

	// Drives one UIInputSystem frame with the pointer at a given UI-space position.
	void TickInput(Scene& scene, const Vector2f& uiPosition, bool mouseDown = false)
	{
		Vector2f viewportMin = Input::GetViewportMin();
		Vector2f viewportSize = Input::GetViewportSize();

		// Convert UI space back to window space, the direction Input works in.
		Input::UpdateMousePosition(Vector2f(
			viewportMin.x + uiPosition.x,
			(viewportMin.y + viewportSize.y) - uiPosition.y));

		Input::SetMouseButtonState(MouseButton::Left, mouseDown);
		Sys<UIInputSystem>()->OnUpdate(Ember::Test::FixedStep(), &scene);
	}

}

EB_TEST_CASE(UI, HoverDrivesTheSelectionStateAndTint, Integration)
{
	SceneFixture scene;
	SizeViewport(*scene);
	Input::SetViewportRect(Vector2f(0.0f), Vector2f(ViewportWidth, ViewportHeight));

	Entity canvas = MakeCanvas(*scene);
	Entity button = MakeButton(*scene, canvas, "Button", Vector2f(200.0f, 100.0f), Vector2f(0.0f));

	auto& selectable = button.GetComponent<UISelectableComponent>();
	selectable.FadeDuration = 0.0f;	// no lerp, so one tick lands exactly on the target colour
	selectable.NormalColor = Vector4f(1.0f, 1.0f, 1.0f, 1.0f);
	selectable.HighlightedColor = Vector4f(1.0f, 0.0f, 0.0f, 1.0f);
	selectable.PressedColor = Vector4f(0.0f, 1.0f, 0.0f, 1.0f);

	Layout(*scene);

	Vector2f centre = RectCentre(button);
	Vector2f outside = centre + Vector2f(400.0f, 0.0f);

	// Pointer away: Normal.
	TickInput(*scene, outside);
	EB_EXPECT_EQ(button.GetComponent<UISelectableComponent>().State, UISelectionState::Normal);

	// Pointer over the button: Highlighted, and the sprite takes the highlight tint.
	TickInput(*scene, centre);
	EB_EXPECT_EQ(button.GetComponent<UISelectableComponent>().State, UISelectionState::Highlighted);
	EB_EXPECT_VEC4_NEAR(button.GetComponent<SpriteComponent>().Color, Vector4f(1.0f, 0.0f, 0.0f, 1.0f), 1e-3f);

	// Held down over the button: Pressed.
	TickInput(*scene, centre, true);
	EB_EXPECT_EQ(button.GetComponent<UISelectableComponent>().State, UISelectionState::Pressed);
	EB_EXPECT_VEC4_NEAR(button.GetComponent<SpriteComponent>().Color, Vector4f(0.0f, 1.0f, 0.0f, 1.0f), 1e-3f);

	// Release over the button completes the click.
	TickInput(*scene, centre, false);
	EB_EXPECT(button.GetComponent<UIButtonComponent>().WasClickedThisFrame);
}

EB_TEST_CASE(UI, SelectableRecoversAfterBeingHiddenAndShown, Integration)
{
	SceneFixture scene;
	SizeViewport(*scene);
	Input::SetViewportRect(Vector2f(0.0f), Vector2f(ViewportWidth, ViewportHeight));

	Entity canvas = MakeCanvas(*scene);
	Entity panel = MakeRect(*scene, canvas, "Panel", Vector2f(400.0f, 300.0f), Vector2f(0.0f));
	Entity button = MakeButton(*scene, panel, "Button", Vector2f(200.0f, 100.0f), Vector2f(0.0f));

	auto& selectable = button.GetComponent<UISelectableComponent>();
	selectable.FadeDuration = 0.0f;
	selectable.HighlightedColor = Vector4f(1.0f, 0.0f, 0.0f, 1.0f);

	Layout(*scene);
	Vector2f centre = RectCentre(button);

	// A menu that starts hidden and is revealed on interaction - the shop-menu pattern.
	panel.SetActive(false, true);
	Layout(*scene);
	TickInput(*scene, centre);

	panel.SetActive(true, true);
	Layout(*scene);
	TickInput(*scene, centre);

	EB_EXPECT_EQ(button.GetComponent<UISelectableComponent>().State, UISelectionState::Highlighted);
	EB_EXPECT_VEC4_NEAR(button.GetComponent<SpriteComponent>().Color, Vector4f(1.0f, 0.0f, 0.0f, 1.0f), 1e-3f);
}

EB_TEST_CASE(UI, ArrowKeyAdoptsAFocusWhenNothingIsFocused, Integration)
{
	SceneFixture scene;
	SizeViewport(*scene);
	Input::SetViewportRect(Vector2f(0.0f), Vector2f(ViewportWidth, ViewportHeight));

	Entity canvas = MakeCanvas(*scene);
	Entity first = MakeButton(*scene, canvas, "First", Vector2f(120.0f, 40.0f), Vector2f(0.0f, 100.0f));
	Entity second = MakeButton(*scene, canvas, "Second", Vector2f(120.0f, 40.0f), Vector2f(0.0f, -100.0f));

	Layout(*scene);

	auto uiInput = Sys<UIInputSystem>();
	uiInput->SetFocusedEntity((EntityID)Constants::Entities::InvalidEntityID);

	// Pointer parked away from both buttons so hover cannot supply the focus.
	Vector2f away = Vector2f(20.0f, 20.0f);

	// Without a bootstrap there is nothing to navigate *from*, so keyboard-only players could
	// never reach a menu at all.
	Input::SetKeyState(KeyCode::Down, true);
	TickInput(*scene, away);
	EB_EXPECT_EQ(uiInput->GetFocusedEntity(), first.GetEntityHandle());
	Input::SetKeyState(KeyCode::Down, false);
	TickInput(*scene, away);

	// A second press then navigates normally: Second sits below First.
	Input::SetKeyState(KeyCode::Down, true);
	TickInput(*scene, away);
	EB_EXPECT_EQ(uiInput->GetFocusedEntity(), second.GetEntityHandle());
	Input::SetKeyState(KeyCode::Down, false);

	// A focused selectable reads as Selected so the focus is actually visible on screen.
	TickInput(*scene, away);
	EB_EXPECT_EQ(second.GetComponent<UISelectableComponent>().State, UISelectionState::Selected);

	uiInput->SetFocusedEntity((EntityID)Constants::Entities::InvalidEntityID);
}
