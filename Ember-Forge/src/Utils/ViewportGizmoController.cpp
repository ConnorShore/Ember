#include "efpch.h"

#include "Utils/ViewportGizmoController.h"

#include "EditorContext.h"

#include <Ember/Asset/Font.h>
#include <Ember/Core/Application.h>
#include <Ember/Input/Input.h>
#include <Ember/Input/InputCode.h>
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

	static bool CalculateTextLocalBounds(const TextComponent& textComponent, Vector2f& outMin, Vector2f& outMax)
	{
		if (textComponent.FontHandle == Constants::InvalidUUID || textComponent.Text.empty())
			return false;

		auto font = Application::Instance().GetAssetManager().GetAsset<Font>(textComponent.FontHandle);
		if (!font || !font->GetAtlasTexture())
			return false;

		const stbtt_bakedchar* glyphData = font->GetGlyphData();
		auto atlasTexture = font->GetAtlasTexture();
		float cursorX = 0.0f;
		float cursorY = 0.0f;
		bool hasGlyph = false;

		outMin = Vector2f(std::numeric_limits<float>::max());
		outMax = Vector2f(std::numeric_limits<float>::lowest());

		for (char c : textComponent.Text)
		{
			if (c < Font::FirstChar || c >= Font::FirstChar + Font::CharCount)
				continue;

			stbtt_aligned_quad quad;
			stbtt_GetBakedQuad(glyphData, atlasTexture->GetWidth(), atlasTexture->GetHeight(), c - Font::FirstChar, &cursorX, &cursorY, &quad, 1);

			float quadWidth = quad.x1 - quad.x0;
			float quadHeight = quad.y1 - quad.y0;
			float localX = quad.x0 + quadWidth * 0.5f;
			float localY = -(quad.y0 + quadHeight * 0.5f);

			Vector2f glyphMin = Vector2f(localX - quadWidth * 0.5f, localY - quadHeight * 0.5f);
			Vector2f glyphMax = Vector2f(localX + quadWidth * 0.5f, localY + quadHeight * 0.5f);

			outMin.x = std::min(outMin.x, glyphMin.x);
			outMin.y = std::min(outMin.y, glyphMin.y);
			outMax.x = std::max(outMax.x, glyphMax.x);
			outMax.y = std::max(outMax.y, glyphMax.y);
			hasGlyph = true;
		}

		return hasGlyph && outMax.x > outMin.x && outMax.y > outMin.y;
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
		ImGuizmo::SetOrthographic(false);
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(viewportBounds[0].x, viewportBounds[0].y, viewportBounds[1].x - viewportBounds[0].x, viewportBounds[1].y - viewportBounds[0].y);

		Matrix4f cameraProjection = camera.GetProjectionMatrix();
		Matrix4f cameraView = camera.GetViewMatrix();

		auto& transformComp = context->SelectedEntity.GetComponent<TransformComponent>();
		Matrix4f transform = transformComp.WorldTransform;

		bool snap = Input::IsKeyPressed(KeyCode::LeftControl);
		float snapValue = (gizmoType == ImGuizmo::OPERATION::ROTATE) ? 45.0f : 0.5f;
		float snapValues[3] = { snapValue, snapValue, snapValue };

		bool isLocal = Input::IsKeyPressed(KeyCode::LeftShift);
		ImGuizmo::MODE currentMode = isLocal ? ImGuizmo::LOCAL : ImGuizmo::WORLD;

		ImGuizmo::Manipulate(&cameraView[0][0], &cameraProjection[0][0],
			(ImGuizmo::OPERATION)gizmoType, currentMode, &transform[0][0],
			nullptr, snap ? snapValues : nullptr);

		if (ImGuizmo::IsUsing())
		{
			Matrix4f localTransform = transform;

			if (context->SelectedEntity.ContainsComponent<RelationshipComponent>())
			{
				auto& relationshipComp = context->SelectedEntity.GetComponent<RelationshipComponent>();
				if (relationshipComp.ParentHandle != Constants::InvalidUUID)
				{
					Entity parent = context->ActiveScene()->GetEntity(relationshipComp.ParentHandle);
					if (parent.GetEntityHandle() != Constants::Entities::InvalidEntityID)
					{
						Matrix4f parentWorld = parent.GetComponent<TransformComponent>().WorldTransform;
						localTransform = Math::Inverse(parentWorld) * transform;
					}
				}
			}

			Vector3f translation, rotation, scale;
			Math::DecomposeTransform(localTransform, translation, rotation, scale);

			switch (gizmoType)
			{
			case ImGuizmo::OPERATION::TRANSLATE:
			{
				transformComp.Position = translation;
				break;
			}
			case ImGuizmo::OPERATION::ROTATE:
			{
				if (!std::isnan(rotation.x) && !std::isnan(rotation.y) && !std::isnan(rotation.z))
					transformComp.Rotation = rotation;
				break;
			}
			case ImGuizmo::OPERATION::SCALE:
			{
				float epsilon = 0.001f;
				if (std::isnan(scale.x) || abs(scale.x) < epsilon) scale.x = epsilon;
				if (std::isnan(scale.y) || abs(scale.y) < epsilon) scale.y = epsilon;
				if (std::isnan(scale.z) || abs(scale.z) < epsilon) scale.z = epsilon;

				transformComp.Scale = scale;
				break;
			}
			case ImGuizmo::OPERATION::UNIVERSAL:
			{
				transformComp.Position = translation;

				if (!std::isnan(rotation.x) && !std::isnan(rotation.y) && !std::isnan(rotation.z))
					transformComp.Rotation = rotation;

				float epsilon = 0.001f;
				if (std::isnan(scale.x) || abs(scale.x) < epsilon) scale.x = epsilon;
				if (std::isnan(scale.y) || abs(scale.y) < epsilon) scale.y = epsilon;
				if (std::isnan(scale.z) || abs(scale.z) < epsilon) scale.z = epsilon;

				transformComp.Scale = scale;
				break;
			}
			}
		}
	}

	void ViewportGizmoController::RenderRectTransformGizmo(EditorContext* context, const Vector2f viewportBounds[2], int gizmoType)
	{
		if (context->SelectedEntity == Constants::Entities::InvalidEntityID || !context->SelectedEntity.ContainsComponent<RectTransformComponent>() || !context->SelectedEntity.ContainsComponent<TransformComponent>())
			return;

		auto& rectTransform = context->SelectedEntity.GetComponent<RectTransformComponent>();
		auto& transform = context->SelectedEntity.GetComponent<TransformComponent>();

		Vector2f localMin = Vector2f(-0.5f);
		Vector2f localMax = Vector2f(0.5f);
		if (context->SelectedEntity.ContainsComponent<TextComponent>())
		{
			Vector2f textMin, textMax;
			if (CalculateTextLocalBounds(context->SelectedEntity.GetComponent<TextComponent>(), textMin, textMax))
			{
				localMin = textMin;
				localMax = textMax;
			}
		}

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