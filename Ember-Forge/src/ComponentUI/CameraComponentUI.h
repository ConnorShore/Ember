#pragma once

#include "ComponentUI.h"

namespace Ember {

	class CameraComponentUI : public ComponentUI<CameraComponent>
	{
	public:
		CameraComponentUI(EditorContext* context) : ComponentUI(context) {}
		inline const char* GetName() const override { return "Camera Component"; }

	protected:
		inline void RenderComponentImpl(CameraComponent& component) override
		{
			ImGui::Text("Camera Component Data goes here");
		}

	private:
	};

}