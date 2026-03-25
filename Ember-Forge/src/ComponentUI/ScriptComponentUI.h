#pragma once

#include "ComponentUI.h"

#include <filesystem>
#include <fstream>

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
			bool scriptExists = true;
			std::string fileName;
			if (component.ScriptHandle != Constants::InvalidUUID)
			{
				auto scriptAsset = assetManager.GetAsset<Script>(component.ScriptHandle);
				if (scriptAsset)
					fileName = std::filesystem::path(scriptAsset->GetFilePath()).filename().string();
			}
			else
			{
				fileName = "No Script";
				scriptExists = false;
			}

			ImGui::Text("Script: %s", fileName.c_str());
			ImGui::SameLine();
			if (ImGui::Button("Select/Drop Script")) 
			{
				std::string scriptFile = FileDialog::OpenFile("Ember-Forge/assets/scripts", "Ember Script (*.lua)", "*.lua");
				if (!scriptFile.empty())
				{
					auto scriptAsset = assetManager.Load<Script>(scriptFile);

					component.ScriptHandle = scriptAsset->GetUUID();
					component.Initialized = false;
				}
			}

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_FILE"))
				{
					EB_CORE_TRACE("Received payload with data: {}", (const char*)payload->Data);
					auto filePath = (const char*)payload->Data;
					auto script = Application::Instance().GetAssetManager().Load<Script>(filePath);
					component.ScriptHandle = script->GetUUID();
					component.Initialized = false;
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::SameLine();
			if (ImGui::Button("Create Script"))
			{
				// Open VS code with a new script file in the scripts directory
				std::string newScriptPath = "Ember-Forge/assets/scripts/NewScript.lua";
				// If script path exists, add a number at the end
				if (std::filesystem::exists(std::filesystem::absolute(newScriptPath)))
				{
					unsigned int index = 1;
					do {
						newScriptPath = std::format("Ember-Forge/assets/scripts/NewScript({}).lua", index++);
						// TODO need to update the scripts name with index (i.e. NewScript_1)
					} while (std::filesystem::exists(std::filesystem::absolute(newScriptPath)));
				}

				std::ofstream newScriptFile(newScriptPath);
				std::string defaultScriptContent = R"(
-- This is a default script template --
local NewScript = {}

function OnCreate()
end
function OnUpdate(deltaTime)
end
function OnDestroy()
end

return NewScript
)";
				newScriptFile << defaultScriptContent;
				newScriptFile.close();

				auto scriptAsset = assetManager.Load<Script>(newScriptPath);
				component.ScriptHandle = scriptAsset->GetUUID();
				component.Initialized = false;

				std::string command = "code " + newScriptPath;
				system(command.c_str());
			}

			if (scriptExists)
			{
				if (ImGui::Button("Edit Script"))
				{
					std::string scriptPath = assetManager.GetAsset<Script>(component.ScriptHandle)->GetFilePath();
					std::string command = "code " + scriptPath;
					system(command.c_str());
				}
				ImGui::SameLine();
				if (ImGui::Button("Clear Script"))
				{
					component.ScriptHandle = Constants::InvalidUUID;
					component.Initialized = false;
				}
			}
		}

	private:
	};

}