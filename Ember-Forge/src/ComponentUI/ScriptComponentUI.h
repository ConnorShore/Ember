#pragma once
#include "ComponentUI.h"
#include "UI/DragDropTypes.h"
#include "UI/PropertyGrid.h"

#include <Ember/Core/ProjectManager.h>
#include <Ember/Utils/PlatformUtil.h>

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
			auto scriptDir = ProjectManager::GetActive()->GetAssetDirectory() / "Scripts";

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
					std::string scriptFile = FileDialog::OpenFile(scriptDir.string().c_str(), "Ember Script (*.lua)", "*.lua");
					if (!scriptFile.empty())
					{
						// If script is within project, keep it where it is, if its outside, copy into the scriptDir
						if (std::filesystem::absolute(scriptFile).string().find(std::filesystem::absolute(ProjectManager::GetActive()->GetAssetDirectory()).string()) != 0)
						{
							std::string destPath = (scriptDir / std::filesystem::path(scriptFile).filename()).string();
							std::filesystem::copy_file(scriptFile, destPath, std::filesystem::copy_options::overwrite_existing);
							droppedPath = destPath;
						}
						else
						{
							droppedPath = scriptFile;
						}

						// TODO: If the script is already an engine asset, we don't want to mark it as a non-engine asset

						auto scriptAsset = assetManager.Load<Script>(scriptFile);
						scriptAsset->SetIsEngineAsset(false); // User-created script, not an engine asset

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
					// Show popup to set script name
					if (ImGui::BeginPopupModal("Create New Script", NULL, ImGuiWindowFlags_AlwaysAutoResize))
					{
						static char scriptName[128] = "NewScript";
						ImGui::InputText("Script Name", scriptName, sizeof(scriptName));
						if (ImGui::Button("Create", ImVec2(120, 0)))
						{
							std::string newScriptPath = (scriptDir / std::format("{}.lua", scriptName)).string();
							if (std::filesystem::exists(std::filesystem::absolute(newScriptPath)))
							{
								ImGui::OpenPopup("File Exists");
							}
							else
							{
								std::ofstream newScriptFile(newScriptPath);
								newScriptFile << "-- Default Script\nlocal " << scriptName << " = {}\nfunction OnCreate(entity)\nend\nfunction OnUpdate(delta)\nend\nreturn " << scriptName;
								newScriptFile.close();
								auto scriptAsset = assetManager.Load<Script>(newScriptPath);
								scriptAsset->SetIsEngineAsset(false); // User-created script, not an engine asset
								component.ScriptHandle = scriptAsset->GetUUID();
								component.Initialized = false;
								system(("code " + newScriptPath).c_str());
								ImGui::CloseCurrentPopup();
							}
						}
						if (ImGui::BeginPopupModal("File Exists", NULL, ImGuiWindowFlags_AlwaysAutoResize))
						{
							ImGui::Text("A script with that name already exists. Please choose a different name.");
							if (ImGui::Button("OK", ImVec2(120, 0)))
							{
								ImGui::CloseCurrentPopup();
							}
							ImGui::EndPopup();
						}
						ImGui::EndPopup();
					}
					else
					{
						ImGui::OpenPopup("Create New Script");
					}

					std::string newScriptPath = (scriptDir / "NewScript.lua").string();
					if (std::filesystem::exists(std::filesystem::absolute(newScriptPath)))
					{
						uint32_t index = 1;
						do {
							newScriptPath = std::format("{}/NewScript({}).lua", scriptDir.string(), index++);
						} while (std::filesystem::exists(std::filesystem::absolute(newScriptPath)));
					}

					std::ofstream newScriptFile(newScriptPath);
					newScriptFile << "-- Default Script\nlocal NewScript = {}\nfunction OnCreate()\nend\nfunction OnUpdate(dt)\nend\nreturn NewScript";
					newScriptFile.close();

					auto scriptAsset = assetManager.Load<Script>(newScriptPath);
					scriptAsset->SetIsEngineAsset(false); // User-created script, not an engine asset

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