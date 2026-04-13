#pragma once

#include "ComponentUI.h"
#include "UI/PropertyGrid.h"
#include "Ember/Math/Math.h"

#include <imgui/imgui.h>

namespace Ember {

	class CapsuleColliderComponentUI : public ComponentUI<CapsuleColliderComponent>
	{
	public:
		CapsuleColliderComponentUI(EditorContext* context) : ComponentUI(context) {}
		inline const char* GetName() const override { return "Capsule Collider"; }

		void CreateComponentForEntity(Entity entity) override
		{
			if (entity.ContainsComponent<CapsuleColliderComponent>())
			{
				EB_CORE_WARN("Entity already contains a component of type %s!", GetName());
				return;
			}

			CapsuleColliderComponent comp;
			m_Context->ActiveScene->AttachComponent(entity, comp);
		}

	protected:
		inline void RenderComponentImpl(CapsuleColliderComponent& component) override
		{
			if (UI::PropertyGrid::Begin("CapsuleColliderProps"))
			{
				UI::PropertyGrid::Float("Radius", component.Radius);
				UI::PropertyGrid::Float("Height", component.Height);
				UI::PropertyGrid::Float3("Offset", component.Offset);

				UI::PropertyGrid::End();
			}
		}
	};

}