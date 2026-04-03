#pragma once
#include "ComponentUI.h"
#include "UI/DragDropTypes.h"
#include "UI/PropertyGrid.h"

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
			bool scriptExists = component.ScriptHandle != Constants::InvalidUUID;
			std::string fileName = "None (Script)";

			if (scriptExists)
			{
				auto scriptAsset = assetManager.GetAsset<Script>(component.ScriptHandle);
				if (scriptAsset)
					fileName = std::filesystem::path(scriptAsset->GetFilePath()).filename().string();
			}

			if (UI::PropertyGrid::Begin("ScriptProps"))
			{
				std::string payloadType = DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::AssetScript);
				std::string droppedPath;

				UI::UICallbackFunc browseFunc = [&]() {
					std::string scriptFile = FileDialog::OpenFile("Ember-Forge/assets/scripts", "Ember Script (*.lua)", "*.lua");
					if (!scriptFile.empty())
					{
						auto scriptAsset = assetManager.Load<Script>(scriptFile);
						component.ScriptHandle = scriptAsset->GetUUID();
						component.Initialized = false;
					}
					};

				UI::UICallbackFunc clearFunc = scriptExists ? UI::UICallbackFunc([&]() {
					component.ScriptHandle = Constants::InvalidUUID;
					component.Initialized = false;
					}) : nullptr; // Passing nullptr hides the X button automatically

				if (UI::PropertyGrid::AssetReference("Script Asset", fileName, payloadType, droppedPath, browseFunc, clearFunc))
				{
					auto script = assetManager.Load<Script>(droppedPath);
					component.ScriptHandle = script->GetUUID();
					component.Initialized = false;
				}

				UI::UICallbackFunc createFunc = [&]() {
					std::string newScriptPath = "Ember-Forge/assets/scripts/NewScript.lua";
					if (std::filesystem::exists(std::filesystem::absolute(newScriptPath)))
					{
						uint32_t index = 1;
						do {
							newScriptPath = std::format("Ember-Forge/assets/scripts/NewScript({}).lua", index++);
						} while (std::filesystem::exists(std::filesystem::absolute(newScriptPath)));
					}

					std::ofstream newScriptFile(newScriptPath);
					newScriptFile << "-- Default Script\nlocal NewScript = {}\nfunction OnCreate()\nend\nfunction OnUpdate(dt)\nend\nreturn NewScript";
					newScriptFile.close();

					auto scriptAsset = assetManager.Load<Script>(newScriptPath);
					component.ScriptHandle = scriptAsset->GetUUID();
					component.Initialized = false;

					system(("code " + newScriptPath).c_str());
					};

				if (scriptExists)
				{
					UI::UICallbackFunc editFunc = [&]() {
						std::string scriptPath = assetManager.GetAsset<Script>(component.ScriptHandle)->GetFilePath();
						system(("code " + scriptPath).c_str());
						};

					UI::PropertyGrid::ActionRow("Actions", "Edit Script", editFunc, "Create New", createFunc);
				}
				else
				{
					UI::PropertyGrid::ActionRow("Actions", "Create New", createFunc);
				}

				UI::PropertyGrid::End();
			}
		}
	};
}