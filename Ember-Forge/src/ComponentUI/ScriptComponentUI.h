#pragma once
#include "ComponentUI.h"
#include "UI/DragDropTypes.h"
#include "UI/PropertyGrid.h"
#include <Ember/Core/ProjectManager.h>
#include <Ember/Utils/PlatformUtil.h>
#include <filesystem>
#include <fstream>
#include <format>

namespace Ember {

	class ScriptComponentUI : public ComponentUI<ScriptComponent>
	{
	public:
		ScriptComponentUI(EditorContext* context) : ComponentUI(context) {}
		inline const char* GetName() const override { return "Script Component"; }

	protected:
		inline void RenderComponentImpl(ScriptComponent& component) override
		{
			auto scriptDir = ProjectManager::GetActive()->GetAssetDirectory() / "Scripts";
			bool openCreateModal = false;

			DrawProperties(component, scriptDir, openCreateModal);
			DrawCreateModal(component, scriptDir, openCreateModal);
		}

	private:
		void DrawProperties(ScriptComponent& component, const std::filesystem::path& scriptDir, bool& openCreateModal)
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
					std::string scriptFile = FileDialog::OpenFile(scriptDir.string().c_str(), "Ember Script (*.lua)", "*.lua");
					if (!scriptFile.empty())
					{
						HandleScriptImport(scriptFile, scriptDir, component);
					}
					};

				UI::UICallbackFunc clearFunc = scriptExists ? UI::UICallbackFunc([&]() {
					component.ScriptHandle = Constants::InvalidUUID;
					component.Initialized = false;
					}) : nullptr;

				if (UI::PropertyGrid::AssetReference("Script Asset", fileName, payloadType, droppedPath, browseFunc, clearFunc))
				{
					auto script = assetManager.Load<Script>(droppedPath);
					component.ScriptHandle = script->GetUUID();
					component.Initialized = false;
				}

				UI::UICallbackFunc createFunc = [&]() { openCreateModal = true; };

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

		void DrawCreateModal(ScriptComponent& component, const std::filesystem::path& scriptDir, bool& openCreateModal)
		{
			if (openCreateModal)
			{
				ImGui::OpenPopup("Create New Script");
			}

			if (ImGui::BeginPopupModal("Create New Script", NULL, ImGuiWindowFlags_AlwaysAutoResize))
			{
				static char scriptName[128] = "NewScript";
				ImGui::InputText("Script Name", scriptName, sizeof(scriptName));

				ImGui::Spacing();

				if (ImGui::Button("Create", ImVec2(120, 0)))
				{
					std::string newScriptPath = (scriptDir / std::format("{}.lua", scriptName)).string();
					if (std::filesystem::exists(std::filesystem::absolute(newScriptPath)))
					{
						ImGui::OpenPopup("File Exists");
					}
					else
					{
						GenerateScriptTemplate(scriptName, newScriptPath, component);

						// Reset buffer and close
						strcpy(scriptName, "NewScript");
						ImGui::CloseCurrentPopup();
					}
				}

				ImGui::SameLine();

				if (ImGui::Button("Cancel", ImVec2(120, 0)))
				{
					ImGui::CloseCurrentPopup();
				}

				// Nested Warning Popup
				if (ImGui::BeginPopupModal("File Exists", NULL, ImGuiWindowFlags_AlwaysAutoResize))
				{
					ImGui::Text("A script with that name already exists.\nPlease choose a different name.");
					ImGui::Spacing();
					if (ImGui::Button("OK", ImVec2(120, 0)))
					{
						ImGui::CloseCurrentPopup();
					}
					ImGui::EndPopup();
				}

				ImGui::EndPopup();
			}
		}

		void HandleScriptImport(const std::string& sourceFile, const std::filesystem::path& scriptDir, ScriptComponent& component)
		{
			auto& assetManager = Application::Instance().GetAssetManager();
			std::string finalPath = sourceFile;

			// If script is outside the project's Asset directory, copy it in
			if (std::filesystem::absolute(sourceFile).string().find(std::filesystem::absolute(ProjectManager::GetActive()->GetAssetDirectory()).string()) != 0)
			{
				finalPath = (scriptDir / std::filesystem::path(sourceFile).filename()).string();
				std::filesystem::copy_file(sourceFile, finalPath, std::filesystem::copy_options::overwrite_existing);
			}

			// Avoid reloading if it's the exact same asset
			SharedPtr<Script> scriptAsset;
			if (assetManager.GetAsset<Script>(component.ScriptHandle) && assetManager.GetAsset<Script>(component.ScriptHandle)->GetFilePath() == finalPath)
			{
				scriptAsset = assetManager.GetAsset<Script>(component.ScriptHandle);
			}
			else
			{
				scriptAsset = assetManager.Load<Script>(finalPath);
				scriptAsset->SetIsEngineAsset(false);
			}

			component.ScriptHandle = scriptAsset->GetUUID();
			component.Initialized = false;
		}

		void GenerateScriptTemplate(const std::string& scriptName, const std::string& filepath, ScriptComponent& component)
		{
			// Script template
			std::ofstream newScriptFile(filepath);
			newScriptFile << "local " << scriptName << " = {}\n\n";
			newScriptFile << "function " << scriptName << ":OnCreate(entity)\n\nend\n\n";
			newScriptFile << "function " << scriptName << ":OnUpdate(entity, delta)\n\nend\n\n";
			newScriptFile << "return " << scriptName;
			newScriptFile.close();

			// Load it
			auto& assetManager = Application::Instance().GetAssetManager();
			auto scriptAsset = assetManager.Load<Script>(filepath);
			scriptAsset->SetIsEngineAsset(false);

			// Update component
			component.ScriptHandle = scriptAsset->GetUUID();
			component.Initialized = false;

			// Open in VS Code (or default editor)
			system(("code " + filepath).c_str());
		}
	};
}