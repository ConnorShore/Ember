#include "efpch.h"

#include "Utils/ViewportGizmoController.h"
#include "Utils/ActiveNavMeshRenderer.h"

#include "EditorContext.h"

#include <Ember/Core/Application.h>
#include <Ember/Input/Input.h>
#include <Ember/Input/InputCode.h>
#include <Ember/Render/DebugRenderer.h>
#include <Ember/Scene/Entity.h>
#include <Ember/Scene/Scene.h>
#include <Ember/Tools/EditorCamera.h>
#include <Ember/Math/Math.h>

#include <ImGuizmo.h>
#include <imgui/imgui.h>

#include <algorithm>
#include <cmath>
#include <limits>

namespace Ember {
	constexpr float kTwoPi = 6.28318530718f;
	constexpr float kLightGizmoMinContribution = 0.05f;

	static float EstimateInverseSquareRange(float intensity, float minContribution = kLightGizmoMinContribution)
	{
		if (intensity <= 0.0f || minContribution <= 0.0f)
			return 0.0f;

		return std::sqrt(intensity / minContribution);
	}

	static Vector3f SafeNormalize(const Vector3f& vector, const Vector3f& fallback)
	{
		if (Math::Magnitude2(vector) <= 1e-6f)
			return fallback;

		return Math::Normalize(vector);
	}

	static void BuildBasisFromDirection(const Vector3f& direction, Vector3f& outRight, Vector3f& outUp)
	{
		Vector3f safeDirection = SafeNormalize(direction, Vector3f(0.0f, 0.0f, -1.0f));
		Vector3f referenceUp = (std::abs(Math::Dot(safeDirection, Vector3f(0.0f, 1.0f, 0.0f))) > 0.99f)
			? Vector3f(1.0f, 0.0f, 0.0f)
			: Vector3f(0.0f, 1.0f, 0.0f);

		outRight = SafeNormalize(Math::Cross(safeDirection, referenceUp), Vector3f(1.0f, 0.0f, 0.0f));
		outUp = SafeNormalize(Math::Cross(outRight, safeDirection), Vector3f(0.0f, 1.0f, 0.0f));
	}

	static void DrawWireCircle(const Vector3f& center, const Vector3f& normal, float radius, const Vector4f& color, int segments = 32)
	{
		if (radius <= 0.0f || segments < 3)
			return;

		Vector3f right, up;
		BuildBasisFromDirection(normal, right, up);

		for (int segmentIndex = 0; segmentIndex < segments; ++segmentIndex)
		{
			float startAngle = (static_cast<float>(segmentIndex) / static_cast<float>(segments)) * kTwoPi;
			float endAngle = (static_cast<float>(segmentIndex + 1) / static_cast<float>(segments)) * kTwoPi;

			Vector3f startPoint = center + (right * Math::Cos(startAngle) + up * Math::Sin(startAngle)) * radius;
			Vector3f endPoint = center + (right * Math::Cos(endAngle) + up * Math::Sin(endAngle)) * radius;
			DebugRenderer::DrawLine(startPoint, endPoint, color);
		}
	}

	static void DrawWireSphere(const Vector3f& center, float radius, const Vector4f& color, int segments = 32)
	{
		DrawWireCircle(center, Vector3f(1.0f, 0.0f, 0.0f), radius, color, segments);
		DrawWireCircle(center, Vector3f(0.0f, 1.0f, 0.0f), radius, color, segments);
		DrawWireCircle(center, Vector3f(0.0f, 0.0f, 1.0f), radius, color, segments);
	}

	static void DrawArrow(const Vector3f& start, const Vector3f& direction, float length, const Vector4f& color)
	{
		if (length <= 0.0f)
			return;

		Vector3f normalizedDirection = SafeNormalize(direction, Vector3f(0.0f, 0.0f, -1.0f));
		Vector3f end = start + normalizedDirection * length;
		DebugRenderer::DrawLine(start, end, color);

		Vector3f right, up;
		BuildBasisFromDirection(normalizedDirection, right, up);

		float headLength = length * 0.2f;
		float headWidth = length * 0.08f;
		Vector3f headBase = end - normalizedDirection * headLength;

		DebugRenderer::DrawLine(end, headBase + right * headWidth, color);
		DebugRenderer::DrawLine(end, headBase - right * headWidth, color);
		DebugRenderer::DrawLine(end, headBase + up * headWidth, color);
		DebugRenderer::DrawLine(end, headBase - up * headWidth, color);
	}

	static void DrawWireCone(const Vector3f& apex, const Vector3f& direction, float length, float halfAngleRadians, const Vector4f& color, int circleSegments = 32, int spokeCount = 8)
	{
		if (length <= 0.0f || halfAngleRadians <= 0.0f)
			return;

		Vector3f normalizedDirection = SafeNormalize(direction, Vector3f(0.0f, 0.0f, -1.0f));
		Vector3f right, up;
		BuildBasisFromDirection(normalizedDirection, right, up);

		float coneRadius = Math::Tan(halfAngleRadians) * length;
		if (coneRadius <= 0.0f)
			return;

		Vector3f baseCenter = apex + normalizedDirection * length;
		DrawWireCircle(baseCenter, normalizedDirection, coneRadius, color, circleSegments);

		if (spokeCount < 4)
			spokeCount = 4;

		for (int spokeIndex = 0; spokeIndex < spokeCount; ++spokeIndex)
		{
			float angle = (static_cast<float>(spokeIndex) / static_cast<float>(spokeCount)) * kTwoPi;
			Vector3f rimPoint = baseCenter + (right * Math::Cos(angle) + up * Math::Sin(angle)) * coneRadius;
			DebugRenderer::DrawLine(apex, rimPoint, color);
		}
	}

	static void DrawSelectedLightGizmo(Scene* scene, EntityID selectedEntity)
	{
		if (selectedEntity == (EntityID)Constants::Entities::InvalidEntityID)
			return;

		auto& registry = scene->GetRegistry();
		if (!registry.ContainsComponent<TransformComponent>(selectedEntity))
			return;

		auto& transform = registry.GetComponent<TransformComponent>(selectedEntity);
		const Vector3f origin = transform.GetWorldPosition();
		const Vector3f forward = SafeNormalize(transform.GetForward(), Vector3f(0.0f, 0.0f, -1.0f));

		if (registry.ContainsComponent<PointLightComponent>(selectedEntity))
		{
			auto& pointLight = registry.GetComponent<PointLightComponent>(selectedEntity);

			float radius = pointLight.Radius > 0.0f ? pointLight.Radius : EstimateInverseSquareRange(pointLight.Intensity);
			if (radius <= 0.0f)
				radius = 1.0f;
			radius = Math::Clamp(radius, 0.1f, 250.0f);

			Vector3f baseColor = Math::Lerp(pointLight.Color, Vector3f(1.0f, 0.85f, 0.2f), 0.35f);
			Vector4f color(baseColor, pointLight.Active ? 1.0f : 0.45f);

			DrawWireSphere(origin, radius, color, 36);
			DebugRenderer::DrawLine(origin - Vector3f(radius, 0.0f, 0.0f), origin + Vector3f(radius, 0.0f, 0.0f), color);
			DebugRenderer::DrawLine(origin - Vector3f(0.0f, radius, 0.0f), origin + Vector3f(0.0f, radius, 0.0f), color);
			DebugRenderer::DrawLine(origin - Vector3f(0.0f, 0.0f, radius), origin + Vector3f(0.0f, 0.0f, radius), color);
		}

		if (registry.ContainsComponent<DirectionalLightComponent>(selectedEntity))
		{
			auto& directionalLight = registry.GetComponent<DirectionalLightComponent>(selectedEntity);

			float arrowLength = Math::Clamp(2.5f + directionalLight.Intensity * 0.25f, 2.5f, 10.0f);
			Vector3f baseColor = Math::Lerp(directionalLight.Color, Vector3f(1.0f, 0.8f, 0.2f), 0.5f);
			Vector4f color(baseColor, directionalLight.Active ? 1.0f : 0.45f);

			DrawArrow(origin, forward, arrowLength, color);

			Vector3f right, up;
			BuildBasisFromDirection(forward, right, up);
			float offset = arrowLength * 0.2f;
			float rayLength = arrowLength * 0.6f;

			DebugRenderer::DrawLine(origin + right * offset, origin + right * offset + forward * rayLength, color);
			DebugRenderer::DrawLine(origin - right * offset, origin - right * offset + forward * rayLength, color);
			DebugRenderer::DrawLine(origin + up * offset, origin + up * offset + forward * rayLength, color);
		}

		if (registry.ContainsComponent<SpotLightComponent>(selectedEntity))
		{
			auto& spotLight = registry.GetComponent<SpotLightComponent>(selectedEntity);

			float coneLength = Math::Clamp(EstimateInverseSquareRange(spotLight.Intensity), 1.0f, 120.0f);
			float outerAngle = Math::Clamp(spotLight.OuterCutOffAngle, 0.001f, Math::Radians(89.0f));
			float innerAngle = Math::Clamp(spotLight.CutOffAngle, 0.001f, outerAngle);

			Vector3f outerTint = Math::Lerp(spotLight.Color, Vector3f(1.0f, 0.78f, 0.18f), 0.45f);
			Vector3f innerTint = Math::Lerp(spotLight.Color, Vector3f(1.0f, 0.55f, 0.1f), 0.25f);

			Vector4f outerColor(outerTint, spotLight.Active ? 1.0f : 0.45f);
			Vector4f innerColor(innerTint, spotLight.Active ? 1.0f : 0.45f);

			DrawWireCone(origin, forward, coneLength, outerAngle, outerColor, 36, 10);
			DrawWireCone(origin, forward, coneLength, innerAngle, innerColor, 24, 6);
			DrawArrow(origin, forward, coneLength * 0.55f, outerColor);
		}
	}

	void ViewportGizmoController::DrawSceneDebugGizmos(Scene* scene, EntityID selectedEntity)
	{
		if (!scene)
			return;

		auto& registry = scene->GetRegistry();
		for (EntityID cameraEntity : registry.ActiveQuery<CameraComponent, TransformComponent>())
		{
			auto [cameraComponent, transform] = registry.GetComponents<CameraComponent, TransformComponent>(cameraEntity);
			cameraComponent.Camera.DrawFrustum(transform.WorldTransform, cameraEntity == selectedEntity);
		}

		DrawSelectedLightGizmo(scene, selectedEntity);
		ActiveNavMeshRenderer::Draw(scene, selectedEntity);
	}

	void ViewportGizmoController::Render(EditorContext* context, EditorCamera& camera, const Vector2f viewportBounds[2], int gizmoType)
	{
		m_RectTransformGizmoHovered = false;

		if (gizmoType == -1 || context->CurrentSceneState != SceneState::Edit)
			return;

		if (context->SelectedEntity == Constants::Entities::InvalidEntityID || !context->SelectedEntity.ContainsComponent<TransformComponent>())
			return;

		if (context->SelectedEntity.ContainsComponent<RectTransformComponent>())
		{
			RenderRectTransformGizmo(context, viewportBounds, gizmoType);
			return;
		}

		RenderTransformGizmo(context, camera, viewportBounds, gizmoType);
	}

	void ViewportGizmoController::RenderTransformGizmo(EditorContext* context, EditorCamera& camera, const Vector2f viewportBounds[2], int gizmoType)
	{
		// ImGuizmo scales its handles by projection type.
		ImGuizmo::SetOrthographic(camera.IsOrthographic());
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(viewportBounds[0].x, viewportBounds[0].y, viewportBounds[1].x - viewportBounds[0].x, viewportBounds[1].y - viewportBounds[0].y);

		Matrix4f cameraProjection = camera.GetProjectionMatrix();
		Matrix4f cameraView = camera.GetViewMatrix();

		// A child whose ancestor is also selected already moves with that ancestor; transforming it
		// again would apply the delta twice.
		std::vector<Entity> targets = FilterOutSelectedDescendants(context);
		if (targets.empty())
			return;

		// The active entity is the pivot, which keeps the single-selection path identical to before.
		Entity pivotEntity = context->SelectedEntity;
		if (!context->IsSelected(pivotEntity) || !pivotEntity.ContainsComponent<TransformComponent>())
			pivotEntity = targets.back();

		Matrix4f pivotWorld = pivotEntity.GetComponent<TransformComponent>().WorldTransform;
		Matrix4f transform = pivotWorld;

		const EditorPreferences& prefs = *context->Preferences;

		// Ctrl inverts the persistent toggle, so it still means "snap" when snapping is switched off.
		bool ctrlHeld = Input::IsModifierActive(KeyModifier::Control);
		bool snap = prefs.SnapEnabled != ctrlHeld;

		float snapValue = gizmoType == ImGuizmo::OPERATION::ROTATE ? prefs.RotateSnap
			: gizmoType == ImGuizmo::OPERATION::SCALE ? prefs.ScaleSnap
			: prefs.TranslateSnap;
		float snapValues[3] = { snapValue, snapValue, snapValue };

		ImGuizmo::MODE currentMode = prefs.GizmoLocalSpace ? ImGuizmo::LOCAL : ImGuizmo::WORLD;

		ImGuizmo::Manipulate(&cameraView[0][0], &cameraProjection[0][0],
			(ImGuizmo::OPERATION)gizmoType, currentMode, &transform[0][0],
			nullptr, snap ? snapValues : nullptr);

		if (!ImGuizmo::IsUsing())
			return;

		if (targets.size() == 1)
		{
			// Single selection keeps the absolute-matrix path: repeated delta multiplication would
			// accumulate float error over a long drag.
			ApplyWorldTransform(context, targets.front(), transform, gizmoType);
			return;
		}

		// The world transform that carries the pivot from its old pose to its new one. Applying it to
		// every target moves the group rigidly, so they orbit and scale about the shared pivot.
		Matrix4f delta = transform * Math::Inverse(pivotWorld);

		for (Entity target : targets)
		{
			auto& targetTransform = target.GetComponent<TransformComponent>();
			ApplyWorldTransform(context, target, delta * targetTransform.WorldTransform, ImGuizmo::OPERATION::UNIVERSAL);
		}
	}

	// Writes a world-space matrix back onto an entity as local TRS, honouring its parent.
	void ViewportGizmoController::ApplyWorldTransform(EditorContext* context, Entity entity, const Matrix4f& worldTransform, int gizmoType)
	{
		auto& transformComp = entity.GetComponent<TransformComponent>();

		Matrix4f localTransform = worldTransform;
		if (entity.ContainsComponent<RelationshipComponent>())
		{
			auto& relationshipComp = entity.GetComponent<RelationshipComponent>();
			if (relationshipComp.ParentHandle != Constants::InvalidUUID)
			{
				Entity parent = context->ActiveScene()->GetEntity(relationshipComp.ParentHandle);
				if (parent.GetEntityHandle() != Constants::Entities::InvalidEntityID)
				{
					Matrix4f parentWorld = parent.GetComponent<TransformComponent>().WorldTransform;
					localTransform = Math::Inverse(parentWorld) * worldTransform;
				}
			}
		}

		Vector3f translation, rotation, scale;
		Math::DecomposeTransform(localTransform, translation, rotation, scale);

		bool writePosition = gizmoType == ImGuizmo::OPERATION::TRANSLATE || gizmoType == ImGuizmo::OPERATION::UNIVERSAL;
		bool writeRotation = gizmoType == ImGuizmo::OPERATION::ROTATE || gizmoType == ImGuizmo::OPERATION::UNIVERSAL;
		bool writeScale = gizmoType == ImGuizmo::OPERATION::SCALE || gizmoType == ImGuizmo::OPERATION::UNIVERSAL;

		if (writePosition)
			transformComp.Position = translation;

		if (writeRotation && !Math::IsNaN(rotation.x) && !Math::IsNaN(rotation.y) && !Math::IsNaN(rotation.z))
			transformComp.Rotation = rotation;

		if (writeScale)
		{
			constexpr float epsilon = 0.001f;
			if (Math::IsNaN(scale.x) || Math::Abs(scale.x) < epsilon) scale.x = epsilon;
			if (Math::IsNaN(scale.y) || Math::Abs(scale.y) < epsilon) scale.y = epsilon;
			if (Math::IsNaN(scale.z) || Math::Abs(scale.z) < epsilon) scale.z = epsilon;

			transformComp.Scale = scale;
		}
	}

	// Drops any selected entity that has a selected ancestor, since hierarchy propagation already
	// moves it with that ancestor.
	std::vector<Entity> ViewportGizmoController::FilterOutSelectedDescendants(EditorContext* context)
	{
		std::vector<Entity> roots = context->ActiveScene()->FilterToHierarchyRoots(context->SelectedEntities);

		std::erase_if(roots, [](Entity entity) { return !entity.ContainsComponent<TransformComponent>(); });
		return roots;
	}

	void ViewportGizmoController::RenderRectTransformGizmo(EditorContext* context, const Vector2f viewportBounds[2], int gizmoType)
	{
		if (context->SelectedEntity == Constants::Entities::InvalidEntityID || !context->SelectedEntity.ContainsComponent<RectTransformComponent>() || !context->SelectedEntity.ContainsComponent<TransformComponent>())
			return;

		auto& rectTransform = context->SelectedEntity.GetComponent<RectTransformComponent>();
		auto& transform = context->SelectedEntity.GetComponent<TransformComponent>();

		// The gizmo is the rect itself, for text as much as anything else - text is sized by its own
		// FontSize now, so fitting the box to the glyphs would no longer match what gets drawn.
		const Vector2f localMin = Vector2f(-0.5f);
		const Vector2f localMax = Vector2f(0.5f);

		Vector2f localSize = localMax - localMin;
		if (localSize.x <= 0.001f || localSize.y <= 0.001f)
			return;

		Vector2f transformOrigin = Vector2f(transform.WorldTransform[3][0], transform.WorldTransform[3][1]);
		Vector2f rightAxis = Vector2f(transform.WorldTransform[0][0], transform.WorldTransform[0][1]);
		Vector2f upAxis = Vector2f(transform.WorldTransform[1][0], transform.WorldTransform[1][1]);

		float rightAxisLength = Math::Length(rightAxis);
		float upAxisLength = Math::Length(upAxis);
		if (rightAxisLength <= 0.001f || upAxisLength <= 0.001f)
			return;

		Vector2f rightDirection = rightAxis / rightAxisLength;
		Vector2f upDirection = upAxis / upAxisLength;

		Vector2f bottomLeft = transformOrigin + rightAxis * localMin.x + upAxis * localMin.y;
		Vector2f bottomRight = transformOrigin + rightAxis * localMax.x + upAxis * localMin.y;
		Vector2f topRight = transformOrigin + rightAxis * localMax.x + upAxis * localMax.y;
		Vector2f topLeft = transformOrigin + rightAxis * localMin.x + upAxis * localMax.y;
		Vector2f center = (bottomLeft + bottomRight + topRight + topLeft) * 0.25f;

		float width = Math::Length(bottomRight - bottomLeft);
		float height = Math::Length(topLeft - bottomLeft);
		if (width <= 0.001f || height <= 0.001f)
			return;

		auto toScreen = [&viewportBounds](const Vector2f& point)
		{
			return ImVec2(viewportBounds[0].x + point.x, viewportBounds[1].y - point.y);
		};

		auto toUI = [&viewportBounds](const ImVec2& point)
		{
			return Vector2f(point.x - viewportBounds[0].x, viewportBounds[1].y - point.y);
		};

		ImVec2 screenCenter = toScreen(center);
		ImVec2 screenBottomLeft = toScreen(bottomLeft);
		ImVec2 screenBottomRight = toScreen(bottomRight);
		ImVec2 screenTopRight = toScreen(topRight);
		ImVec2 screenTopLeft = toScreen(topLeft);
		ImVec2 screenLeft = ImVec2((screenBottomLeft.x + screenTopLeft.x) * 0.5f, (screenBottomLeft.y + screenTopLeft.y) * 0.5f);
		ImVec2 screenRight = ImVec2((screenBottomRight.x + screenTopRight.x) * 0.5f, (screenBottomRight.y + screenTopRight.y) * 0.5f);
		ImVec2 screenTop = ImVec2((screenTopLeft.x + screenTopRight.x) * 0.5f, (screenTopLeft.y + screenTopRight.y) * 0.5f);
		ImVec2 screenBottom = ImVec2((screenBottomLeft.x + screenBottomRight.x) * 0.5f, (screenBottomLeft.y + screenBottomRight.y) * 0.5f);

		Vector2f rotationHandleDirection = Vector2f(screenTop.x - screenCenter.x, screenTop.y - screenCenter.y);
		float rotationHandleLength = Math::Length(rotationHandleDirection);
		if (rotationHandleLength > 0.001f)
			rotationHandleDirection /= rotationHandleLength;
		else
			rotationHandleDirection = Vector2f(0.0f, -1.0f);
		ImVec2 screenRotation = ImVec2(screenTop.x + rotationHandleDirection.x * 32.0f, screenTop.y + rotationHandleDirection.y * 32.0f);

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const ImU32 outlineColor = IM_COL32(255, 210, 55, 255);
		const ImU32 handleColor = IM_COL32(255, 210, 55, 255);
		const ImU32 handleHoverColor = IM_COL32(255, 245, 180, 255);
		const ImU32 moveColor = IM_COL32(80, 185, 255, 255);
		const ImU32 rotationColor = IM_COL32(120, 220, 130, 255);

		drawList->AddLine(screenBottomLeft, screenBottomRight, outlineColor, 2.0f);
		drawList->AddLine(screenBottomRight, screenTopRight, outlineColor, 2.0f);
		drawList->AddLine(screenTopRight, screenTopLeft, outlineColor, 2.0f);
		drawList->AddLine(screenTopLeft, screenBottomLeft, outlineColor, 2.0f);

		bool canMove = gizmoType == ImGuizmo::OPERATION::TRANSLATE || gizmoType == ImGuizmo::OPERATION::UNIVERSAL;
		bool canResize = gizmoType == ImGuizmo::OPERATION::SCALE || gizmoType == ImGuizmo::OPERATION::UNIVERSAL;
		bool canRotate = gizmoType == ImGuizmo::OPERATION::ROTATE || gizmoType == ImGuizmo::OPERATION::UNIVERSAL;

		ImVec2 mousePosition = ImGui::GetMousePos();
		Vector2f mouseUIPosition = toUI(mousePosition);
		bool mouseInViewport = mousePosition.x >= viewportBounds[0].x && mousePosition.y >= viewportBounds[0].y
			&& mousePosition.x <= viewportBounds[1].x && mousePosition.y <= viewportBounds[1].y;

		auto distanceSquared = [](const ImVec2& a, const ImVec2& b)
		{
			float dx = a.x - b.x;
			float dy = a.y - b.y;
			return dx * dx + dy * dy;
		};

		auto isNear = [&](const ImVec2& point, float radius)
		{
			return distanceSquared(mousePosition, point) <= radius * radius;
		};

		auto isInsideRect = [&]()
		{
			Vector2f local = mouseUIPosition - center;
			float localX = Math::Dot(local, rightDirection);
			float localY = Math::Dot(local, upDirection);
			return std::abs(localX) <= width * 0.5f && std::abs(localY) <= height * 0.5f;
		};

		RectTransformGizmoHandle hoveredHandle = RectTransformGizmoHandle::None;
		if (mouseInViewport)
		{
			if (canRotate && isNear(screenRotation, 10.0f))
				hoveredHandle = RectTransformGizmoHandle::Rotate;
			else if (canResize && isNear(screenTopLeft, 8.0f))
				hoveredHandle = RectTransformGizmoHandle::TopLeft;
			else if (canResize && isNear(screenTopRight, 8.0f))
				hoveredHandle = RectTransformGizmoHandle::TopRight;
			else if (canResize && isNear(screenBottomLeft, 8.0f))
				hoveredHandle = RectTransformGizmoHandle::BottomLeft;
			else if (canResize && isNear(screenBottomRight, 8.0f))
				hoveredHandle = RectTransformGizmoHandle::BottomRight;
			else if (canResize && isNear(screenLeft, 7.0f))
				hoveredHandle = RectTransformGizmoHandle::Left;
			else if (canResize && isNear(screenRight, 7.0f))
				hoveredHandle = RectTransformGizmoHandle::Right;
			else if (canResize && isNear(screenTop, 7.0f))
				hoveredHandle = RectTransformGizmoHandle::Top;
			else if (canResize && isNear(screenBottom, 7.0f))
				hoveredHandle = RectTransformGizmoHandle::Bottom;
			else if (canMove && isInsideRect())
				hoveredHandle = RectTransformGizmoHandle::Move;
		}

		m_RectTransformGizmoHovered = hoveredHandle != RectTransformGizmoHandle::None || m_RectTransformGizmoActiveHandle != RectTransformGizmoHandle::None;

		auto drawHandle = [&](const ImVec2& point, RectTransformGizmoHandle handle)
		{
			ImU32 color = hoveredHandle == handle || m_RectTransformGizmoActiveHandle == handle ? handleHoverColor : handleColor;
			drawList->AddRectFilled(ImVec2(point.x - 4.0f, point.y - 4.0f), ImVec2(point.x + 4.0f, point.y + 4.0f), color);
			drawList->AddRect(ImVec2(point.x - 4.0f, point.y - 4.0f), ImVec2(point.x + 4.0f, point.y + 4.0f), IM_COL32(30, 30, 30, 255));
		};

		if (canResize)
		{
			drawHandle(screenTopLeft, RectTransformGizmoHandle::TopLeft);
			drawHandle(screenTopRight, RectTransformGizmoHandle::TopRight);
			drawHandle(screenBottomLeft, RectTransformGizmoHandle::BottomLeft);
			drawHandle(screenBottomRight, RectTransformGizmoHandle::BottomRight);
			drawHandle(screenLeft, RectTransformGizmoHandle::Left);
			drawHandle(screenRight, RectTransformGizmoHandle::Right);
			drawHandle(screenTop, RectTransformGizmoHandle::Top);
			drawHandle(screenBottom, RectTransformGizmoHandle::Bottom);
		}

		if (canMove)
		{
			drawList->AddCircleFilled(screenCenter, 4.0f, hoveredHandle == RectTransformGizmoHandle::Move ? handleHoverColor : moveColor);
			drawList->AddLine(ImVec2(screenCenter.x - 8.0f, screenCenter.y), ImVec2(screenCenter.x + 8.0f, screenCenter.y), moveColor, 1.5f);
			drawList->AddLine(ImVec2(screenCenter.x, screenCenter.y - 8.0f), ImVec2(screenCenter.x, screenCenter.y + 8.0f), moveColor, 1.5f);
		}

		if (canRotate)
		{
			drawList->AddLine(screenTop, screenRotation, rotationColor, 1.5f);
			drawList->AddCircle(screenRotation, 7.0f, hoveredHandle == RectTransformGizmoHandle::Rotate ? handleHoverColor : rotationColor, 16, 2.0f);
		}

		EntityID selectedEntity = context->SelectedEntity.GetEntityHandle();
		if (m_RectTransformGizmoEntity != selectedEntity && m_RectTransformGizmoActiveHandle != RectTransformGizmoHandle::None)
		{
			m_RectTransformGizmoActiveHandle = RectTransformGizmoHandle::None;
			m_RectTransformGizmoEntity = Constants::Entities::InvalidEntityID;
		}

		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && hoveredHandle != RectTransformGizmoHandle::None)
		{
			m_RectTransformGizmoActiveHandle = hoveredHandle;
			m_RectTransformGizmoEntity = selectedEntity;
			m_RectTransformGizmoLastMousePosition = mouseUIPosition;
		}

		if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
		{
			m_RectTransformGizmoActiveHandle = RectTransformGizmoHandle::None;
			m_RectTransformGizmoEntity = Constants::Entities::InvalidEntityID;
		}

		if (m_RectTransformGizmoActiveHandle == RectTransformGizmoHandle::None || m_RectTransformGizmoEntity != selectedEntity || !ImGui::IsMouseDown(ImGuiMouseButton_Left))
			return;

		Vector2f mouseDelta = mouseUIPosition - m_RectTransformGizmoLastMousePosition;
		m_RectTransformGizmoLastMousePosition = mouseUIPosition;
		if (Math::Length(mouseDelta) <= 0.001f)
			return;

		if (m_RectTransformGizmoActiveHandle == RectTransformGizmoHandle::Move)
		{
			rectTransform.AnchoredPosition += mouseDelta;
			return;
		}

		if (m_RectTransformGizmoActiveHandle == RectTransformGizmoHandle::Rotate)
		{
			Vector2f previousMouse = mouseUIPosition - mouseDelta;
			Vector2f previousDirection = previousMouse - center;
			Vector2f currentDirection = mouseUIPosition - center;
			if (Math::Length(previousDirection) > 0.001f && Math::Length(currentDirection) > 0.001f)
			{
				float previousAngle = std::atan2(previousDirection.y, previousDirection.x);
				float currentAngle = std::atan2(currentDirection.y, currentDirection.x);
				rectTransform.Rotation += currentAngle - previousAngle;
			}
			return;
		}

		const float minSize = 4.0f;
		Vector2f anchoredPositionDelta = Vector2f(0.0f);
		Vector2f sizeDelta = Vector2f(0.0f);
		float localDeltaX = Math::Dot(mouseDelta, rightDirection);
		float localDeltaY = Math::Dot(mouseDelta, upDirection);
		float layoutX = 0.5f - rectTransform.Pivot.x;
		float layoutY = 0.5f - rectTransform.Pivot.y;

		auto applyLeft = [&]()
		{
			float displaySizeChange = Math::Max(-localDeltaX, minSize - width);
			float sizeValueChange = displaySizeChange / localSize.x;
			sizeDelta.x += sizeValueChange;
			anchoredPositionDelta += rightDirection * (-(layoutX + localMax.x) * sizeValueChange);
		};

		auto applyRight = [&]()
		{
			float displaySizeChange = Math::Max(localDeltaX, minSize - width);
			float sizeValueChange = displaySizeChange / localSize.x;
			sizeDelta.x += sizeValueChange;
			anchoredPositionDelta += rightDirection * (-(layoutX + localMin.x) * sizeValueChange);
		};

		auto applyBottom = [&]()
		{
			float displaySizeChange = std::max(-localDeltaY, minSize - height);
			float sizeValueChange = displaySizeChange / localSize.y;
			sizeDelta.y += sizeValueChange;
			anchoredPositionDelta += upDirection * (-(layoutY + localMax.y) * sizeValueChange);
		};

		auto applyTop = [&]()
		{
			float displaySizeChange = std::max(localDeltaY, minSize - height);
			float sizeValueChange = displaySizeChange / localSize.y;
			sizeDelta.y += sizeValueChange;
			anchoredPositionDelta += upDirection * (-(layoutY + localMin.y) * sizeValueChange);
		};

		switch (m_RectTransformGizmoActiveHandle)
		{
		case RectTransformGizmoHandle::Left:
			applyLeft();
			break;
		case RectTransformGizmoHandle::Right:
			applyRight();
			break;
		case RectTransformGizmoHandle::Top:
			applyTop();
			break;
		case RectTransformGizmoHandle::Bottom:
			applyBottom();
			break;
		case RectTransformGizmoHandle::TopLeft:
			applyLeft();
			applyTop();
			break;
		case RectTransformGizmoHandle::TopRight:
			applyRight();
			applyTop();
			break;
		case RectTransformGizmoHandle::BottomLeft:
			applyLeft();
			applyBottom();
			break;
		case RectTransformGizmoHandle::BottomRight:
			applyRight();
			applyBottom();
			break;
		default:
			break;
		}

		rectTransform.SizeDelta += sizeDelta;
		rectTransform.AnchoredPosition += anchoredPositionDelta;
	}

}