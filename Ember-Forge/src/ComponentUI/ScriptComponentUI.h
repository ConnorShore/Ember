#pragma once

#include "ComponentUI.h"

namespace Ember {

	class ScriptComponentUI : public ComponentUI<ScriptComponent>
	{
	public:
		inline const std::string& GetName() const override { return "Script Component"; }

	protected:
		inline void DrawComponentImpl(ScriptComponent& component) override
		{
			if (ImGui::Button("Edit Script")) 
			{

			}
		}

	private:
	};

}