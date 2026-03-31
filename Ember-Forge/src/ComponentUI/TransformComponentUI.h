#pragma once

#include "ComponentUI.h"
#include "Ui/PropertyGrid.h"

namespace Ember {

	class TransformComponentUI : public ComponentUI<TransformComponent>
	{
	public:
		TransformComponentUI(EditorContext* context) : ComponentUI(context) { m_CanRemove = false; }
		inline const char* GetName() const override { return "Transform Component"; }

	protected:
		inline void RenderComponentImpl(TransformComponent& component) override
		{
			if (UI::PropertyGrid::Begin("TransformProps"))
			{
				UI::PropertyGrid::Float3("Position", component.Position);

				Vector3f rotationDegrees = Vector3f(
					Math::Degrees(component.Rotation.x),
					Math::Degrees(component.Rotation.y),
					Math::Degrees(component.Rotation.z)
				);
				
				if (UI::PropertyGrid::Float3("Rotation", rotationDegrees, 1.0f))
				{
					component.Rotation = Vector3f(
						Math::Radians(rotationDegrees.x),
						Math::Radians(rotationDegrees.y),
						Math::Radians(rotationDegrees.z)
					);
				}

				UI::PropertyGrid::Float3("Scale", component.Scale);

				UI::PropertyGrid::End();
			}
		}
	};

}