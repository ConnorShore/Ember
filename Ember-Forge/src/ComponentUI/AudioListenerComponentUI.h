#pragma once

#include "ComponentUI.h"
#include "Ui/PropertyGrid.h"

namespace Ember {

	class AudioListenerComponentUI : public ComponentUI<AudioListenerComponent>
	{
	public:
		AudioListenerComponentUI(EditorContext* context) : ComponentUI(context) { m_CanRemove = false; }
		inline const char* GetName() const override { return "Audio Listener Component"; }

	protected:
		inline void RenderComponentImpl(AudioListenerComponent& component) override
		{
			if (UI::PropertyGrid::Begin("AudioListenerProps"))
			{
				UI::PropertyGrid::Checkbox("Active", component.IsActive);
				UI::PropertyGrid::UInt("Listener ID", component.ListenerIndex);
				UI::PropertyGrid::End();
			}
		}
	};

}