#pragma once

#include "ComponentUI.h"

#include <filesystem>

namespace Ember {

	class ScriptComponentUI : public ComponentUI<ScriptComponent>
	{
	public:
		ScriptComponentUI(EditorContext* context) : ComponentUI(context) {}
		inline const char* GetName() const override { return "Script Component"; }

	protected:
		inline void RenderComponentImpl(ScriptComponent& component) override
		{
			std::string fileName = component.FilePath.empty() ? "No script" : std::filesystem::path(component.FilePath).stem().string();
			ImGui::Text("Script: %s", fileName.c_str());
			ImGui::SameLine();
			if (ImGui::Button("Select Script")) 
			{
				std::string scriptFile = FileDialog::OpenFile("Ember-Forge/assets/scripts", "Ember Script (*.lua)", "*.lua");
				component.FilePath = scriptFile;
				component.Initialized = false; // Mark as uninitialized so it will reload in the Script
			}
		}

	private:
	};

}