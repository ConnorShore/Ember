#pragma once
#include "ComponentUI.h"
#include "UI/PropertyGrid.h"
#include "Ember/Math/Math.h"

#include <imgui/imgui.h>

namespace Ember {

	class BoxColliderComponentUI : public ComponentUI<BoxColliderComponent>
	{
	public:
		BoxColliderComponentUI(EditorContext* context) : ComponentUI(context) {}
		inline const char* GetName() const override { return "Box Collider"; }

		void CreateComponentForEntity(Entity entity) override
		{
			if (entity.ContainsComponent<BoxColliderComponent>())
			{
				EB_CORE_WARN("Entity already contains a component of type %s!", GetName());
				return;
			}

			BoxColliderComponent comp = {};

			// Seed Size from the entity's current world scale so the inspector
			// immediately reflects the visible dimensions of the object
			//if (entity.ContainsComponent<TransformComponent>())
			//{
			//	auto& transform = entity.GetComponent<TransformComponent>();
			//	Vector3f worldPos, worldRot, worldScale;
			//	Math::DecomposeTransform(transform.WorldTransform, worldPos, worldRot, worldScale);
			//	comp.Size = worldScale;
			//}

			m_Context->ActiveScene->AttachComponent(entity, comp);
		}

	protected:
		inline void RenderComponentImpl(BoxColliderComponent& component) override
		{
			if (UI::PropertyGrid::Begin("BoxColliderProps"))
			{
				UI::PropertyGrid::Float3("Size", component.Size);
				UI::PropertyGrid::Float3("Offset", component.Offset);

				UI::PropertyGrid::End();
			}
		}
	};

}