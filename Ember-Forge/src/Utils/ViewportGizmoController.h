#pragma once

#include <Ember/Core/Constants.h>
#include <Ember/ECS/Types.h>
#include <Ember/Math/Math.h>

namespace Ember {

	struct EditorContext;
	class EditorCamera;
	class Scene;

	class ViewportGizmoController
	{
	public:
		void Render(EditorContext* context, EditorCamera& camera, const Vector2f viewportBounds[2], int gizmoType);
		static void DrawSceneDebugGizmos(Scene* scene, EntityID selectedEntity);
		static void SetDrawSelectedNavMeshDebug(bool enabled);
		static bool GetDrawSelectedNavMeshDebug();
		bool IsHovered() const { return m_RectTransformGizmoHovered; }

	private:
		enum class RectTransformGizmoHandle
		{
			None,
			Move,
			Left,
			Right,
			Top,
			Bottom,
			TopLeft,
			TopRight,
			BottomLeft,
			BottomRight,
			Rotate
		};

		void RenderTransformGizmo(EditorContext* context, EditorCamera& camera, const Vector2f viewportBounds[2], int gizmoType);
		void RenderRectTransformGizmo(EditorContext* context, const Vector2f viewportBounds[2], int gizmoType);

	private:
		bool m_RectTransformGizmoHovered = false;
		EntityID m_RectTransformGizmoEntity = Constants::Entities::InvalidEntityID;
		RectTransformGizmoHandle m_RectTransformGizmoActiveHandle = RectTransformGizmoHandle::None;
		Vector2f m_RectTransformGizmoLastMousePosition = Vector2f(0.0f);
	};

}