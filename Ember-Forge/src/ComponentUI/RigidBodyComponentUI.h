#pragma once

#include "ComponentUI.h"
#include "UI/PropertyGrid.h"

namespace Ember {

	static std::string BodyTypeToString(RigidBodyComponent::BodyType type)
	{
		switch (type)
		{
		case RigidBodyComponent::BodyType::Static: return "Static";
		case RigidBodyComponent::BodyType::Dynamic: return "Dynamic";
		case RigidBodyComponent::BodyType::Kinematic: return "Kinematic";
		default: return "Unknown";
		}
	}

	class RigidBodyComponentUI : public ComponentUI<RigidBodyComponent>
	{
	public:
		RigidBodyComponentUI(EditorContext* context) : ComponentUI(context) {}
		inline const char* GetName() const override { return "RigidBody Component"; }

	protected:
		inline void RenderComponentImpl(RigidBodyComponent& component) override
		{
			if (UI::PropertyGrid::Begin("RigidBodyProps"))
			{
				std::string bodyTypeStr = BodyTypeToString(component.Type);
				if (UI::PropertyGrid::BeginComboBox("Body Type", bodyTypeStr))
				{
					for (int i = 0; i < 3; i++)
					{
						auto type = static_cast<RigidBodyComponent::BodyType>(i);
						bool selected = (component.Type == type);
						if (UI::PropertyGrid::ComboBoxItem(BodyTypeToString(type), selected))
						{
							component.Type = type;
						}
					}

					UI::PropertyGrid::EndComboBox();
				}

				// Dynamic body properties
				if (component.Type == RigidBodyComponent::BodyType::Dynamic)
				{
					UI::PropertyGrid::Float("Mass", component.Mass, 0.1f, 0.0f, 10000.0f);
					UI::PropertyGrid::Checkbox("Gravity Enabled", component.GravityEnabled);
				}

				UI::PropertyGrid::End();
			}
		}

	private:
	};

}