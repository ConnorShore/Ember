#pragma once

#include "ComponentUI.h"
#include "Ui/PropertyGrid.h"

namespace Ember {

	class ParticleEmitterComponentUI : public ComponentUI<ParticleEmitterComponent>
	{
	public:
		ParticleEmitterComponentUI(EditorContext* context) : ComponentUI(context) { m_CanRemove = false; }
		inline const char* GetName() const override { return "Particle Emitter Component"; }

	protected:
		inline void RenderComponentImpl(ParticleEmitterComponent& component) override
		{
			if (UI::PropertyGrid::Begin("ParticleEmitterProps"))
			{
				UI::PropertyGrid::Checkbox("Active", component.IsActive);

				UI::PropertyGrid::Float("Emission Rate", component.EmissionRate);

				UI::PropertyGrid::Float3("Velocity", component.Velocity);
				UI::PropertyGrid::Float3("Velocity Variation", component.VelocityVariation);

				UI::PropertyGrid::Color4("Color Begin", component.ColorBegin);
				UI::PropertyGrid::Color4("Color End", component.ColorEnd);

				UI::PropertyGrid::Float("Scale Begin", component.ScaleBegin);
				UI::PropertyGrid::Float("Scale End", component.ScaleEnd);
				UI::PropertyGrid::Float("Scale Variation", component.ScaleVariation);

				UI::PropertyGrid::Float("Lifetime", component.LifeTime);
				UI::PropertyGrid::Float("Lifetime Variation", component.LifeTimeVariation);

				// TOOD: Add texture slot with drag-and-drop support

				UI::PropertyGrid::End();
			}
		}
	};

}