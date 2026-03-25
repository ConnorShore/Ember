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
			auto& assetManager = Application::Instance().GetAssetManager();
			std::string fileName;
			if (component.ScriptHandle != Constants::InvalidUUID)
			{
				auto scriptAsset = assetManager.GetAsset<Script>(component.ScriptHandle);
				if (scriptAsset)
					fileName = std::filesystem::path(scriptAsset->GetFilePath()).filename().string();
			}
			else 
				fileName = "No Script";

			ImGui::Text("Script: %s", fileName.c_str());
			ImGui::SameLine();
			if (ImGui::Button("Select Script")) 
			{
				std::string scriptFile = FileDialog::OpenFile("Ember-Forge/assets/scripts", "Ember Script (*.lua)", "*.lua");
				if (!scriptFile.empty())
				{
					auto scriptAsset = assetManager.Load<Script>(scriptFile);

					component.ScriptHandle = scriptAsset->GetUUID();
					component.Initialized = false;
				}
			}
		}

	private:
	};

}