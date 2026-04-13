#pragma once

#include "ComponentUI.h"
#include "UI/PropertyGrid.h"
#include "Ember/Math/Math.h"

#include <imgui/imgui.h>

namespace Ember {

	class SphereColliderComponentUI : public ComponentUI<SphereColliderComponent>
	{
	public:
		SphereColliderComponentUI(EditorContext* context) : ComponentUI(context) {}
		inline const char* GetName() const override { return "Sphere Collider"; }

		void CreateComponentForEntity(Entity entity) override
		{
			if (entity.ContainsComponent<SphereColliderComponent>())
			{
				EB_CORE_WARN("Entity already contains a component of type %s!", GetName());
				return;
			}

			SphereColliderComponent comp;
			m_Context->ActiveScene->AttachComponent(entity, comp);
		}

	protected:
		inline void RenderComponentImpl(SphereColliderComponent& component) override
		{
			if (UI::PropertyGrid::Begin("SphereColliderProps"))
			{
				UI::PropertyGrid::Float("Radius", component.Radius);
				UI::PropertyGrid::Float3("Offset", component.Offset);

				UI::PropertyGrid::End();
			}
		}
	};

}