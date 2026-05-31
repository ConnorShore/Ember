#pragma once

#include "ComponentUI.h"
#include "UI/DragDropTypes.h"
#include "UI/PropertyGrid.h"

#include <Ember/Core/ProjectManager.h>
#include <Ember/Script/ScriptEngine.h>
#include <Ember/Utils/PlatformUtil.h>
#include <imgui/imgui.h>
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
			RenderExposedScriptProperties(component);
		}

	private:
		void DrawProperties(ScriptComponent& component, const std::filesystem::path& scriptDir, bool& openCreateModal)
		{
			bool scriptExists = component.ScriptHandle != Constants::InvalidUUID;

			std::string fileName = "None (Script)";
			if (scriptExists)
			{
				auto scriptAsset = m_AssetManager.GetAsset<Script>(component.ScriptHandle);
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
					auto script = m_AssetManager.Load<Script>(droppedPath);
					component.ScriptHandle = script->GetUUID();
					component.Initialized = false;
				}

				UI::UICallbackFunc createFunc = [&]() { openCreateModal = true; };

				if (scriptExists)
				{
					UI::UICallbackFunc editFunc = [&]() {
						std::string scriptPath = m_AssetManager.GetAsset<Script>(component.ScriptHandle)->GetFilePath();
						ScriptEditor::OpenScript(scriptPath);
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
						strcpy_s(scriptName, "NewScript");
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
			std::string finalPath = sourceFile;

			// If script is outside the project's Asset directory, copy it in
			if (std::filesystem::absolute(sourceFile).string().find(std::filesystem::absolute(ProjectManager::GetActive()->GetAssetDirectory()).string()) != 0)
			{
				finalPath = (scriptDir / std::filesystem::path(sourceFile).filename()).string();
				std::filesystem::copy_file(sourceFile, finalPath, std::filesystem::copy_options::overwrite_existing);
			}

			// Avoid reloading if it's the exact same asset
			SharedPtr<Script> scriptAsset;
			if (m_AssetManager.GetAsset<Script>(component.ScriptHandle) && m_AssetManager.GetAsset<Script>(component.ScriptHandle)->GetFilePath() == finalPath)
			{
				scriptAsset = m_AssetManager.GetAsset<Script>(component.ScriptHandle);
			}
			else
			{
				scriptAsset = m_AssetManager.Load<Script>(finalPath);
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
			newScriptFile << "-- Expose properties to the editor by adding them to this table. For Example:\n";
			newScriptFile << "-- " <<  scriptName << ".MyExampleVar = 10\n\n";
			newScriptFile << "function " << scriptName << ":OnCreate(entity)\n\nend\n\n";
			newScriptFile << "function " << scriptName << ":OnUpdate(entity, delta)\n\nend\n\n";
			newScriptFile << "return " << scriptName;
			newScriptFile.close();

			// Load it
			auto scriptAsset = m_AssetManager.Load<Script>(filepath);
			scriptAsset->SetIsEngineAsset(false);

			// Update component
			component.ScriptHandle = scriptAsset->GetUUID();
			component.Initialized = false;

			// Open in VS Code (or default editor)
			ScriptEditor::OpenScript(filepath);
		}

		void RefreshScriptProperties(ScriptComponent& component, const SharedPtr<Script>& scriptAsset)
		{
			// Re-parse the .lua file so newly added / removed properties are picked up.
			scriptAsset->SetExposedProperties(ScriptEngine::GetScriptProperties(scriptAsset));

			// Drop overrides that no longer correspond to an exposed property of the same type.
			const auto& props = scriptAsset->GetExposedProperties();
			for (auto it = component.UserPropertyOverrides.begin(); it != component.UserPropertyOverrides.end(); )
			{
				auto match = std::find_if(props.begin(), props.end(),
					[&](const ScriptProperty& p) {
						if (p.Name != it->first || p.Type != it->second.Type)
							return false;

						if (p.Type == ScriptPropertyType::EntityRef || p.Type == ScriptPropertyType::AssetRef)
							return it->second.ReferenceKind == ScriptReferenceKind::None || p.ReferenceKind == it->second.ReferenceKind;

						return true;
					});
				if (match == props.end())
					it = component.UserPropertyOverrides.erase(it);
				else
					++it;
			}

			component.Initialized = false;
		}

		std::string ResolveEntityReferenceName(UUID entityUUID) const
		{
			if (entityUUID == Constants::InvalidUUID)
				return "None (Entity)";

			if (!m_Context || !m_Context->ActiveScene())
				return "Missing Entity";

			auto scene = m_Context->ActiveScene();
			std::string entityName;
			return scene->TryGetEntityName(entityUUID, entityName) ? entityName : "Missing Entity";
		}

		std::string ResolveAssetReferenceName(UUID assetUUID, ScriptReferenceKind kind) const
		{
			if (assetUUID == Constants::InvalidUUID)
				return std::format("None ({})", ScriptReferenceKindToString(kind));

			if (!m_AssetManager.ContainsAsset(assetUUID))
				return "Missing Asset";

			return m_AssetManager.GetAssetBase(assetUUID)->GetName();
		}

		std::string ReferencePayloadType(ScriptReferenceKind kind) const
		{
			switch (kind)
			{
			case ScriptReferenceKind::Texture: return DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::AssetTexture);
			case ScriptReferenceKind::Mesh: return DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::AssetMesh);
			case ScriptReferenceKind::Model: return DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::AssetModel);
			case ScriptReferenceKind::Script: return DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::AssetScript);
			case ScriptReferenceKind::Shader: return DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::AssetShader);
			case ScriptReferenceKind::Material: return DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::AssetMaterial);
			case ScriptReferenceKind::PhysicsMaterial: return DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::AssetPhysicsMaterial);
			case ScriptReferenceKind::Prefab: return DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::AssetPrefab);
			case ScriptReferenceKind::Font: return DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::AssetFont);
			case ScriptReferenceKind::AudioClip: return DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::AssetAudioClip);
			case ScriptReferenceKind::Scene: return DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::Scene);
			default: return "";
			}
		}

		UUID LoadDroppedReferenceAsset(ScriptReferenceKind kind, const std::string& droppedPath)
		{
			switch (kind)
			{
			case ScriptReferenceKind::Texture: return m_AssetManager.Load<Texture2D>(droppedPath, false)->GetUUID();
			case ScriptReferenceKind::Mesh: return m_AssetManager.Load<Mesh>(droppedPath, false)->GetUUID();
			case ScriptReferenceKind::Model: return m_AssetManager.Load<Model>(droppedPath, false)->GetUUID();
			case ScriptReferenceKind::Script: return m_AssetManager.Load<Script>(droppedPath, false)->GetUUID();
			case ScriptReferenceKind::Shader: return m_AssetManager.Load<Shader>(droppedPath, false)->GetUUID();
			case ScriptReferenceKind::Material: return m_AssetManager.Load<MaterialBase>(droppedPath, false)->GetUUID();
			case ScriptReferenceKind::PhysicsMaterial: return m_AssetManager.Load<PhysicsMaterial>(droppedPath, false)->GetUUID();
			case ScriptReferenceKind::Prefab: return m_AssetManager.Load<Prefab>(droppedPath, false)->GetUUID();
			case ScriptReferenceKind::Font: return m_AssetManager.Load<Font>(droppedPath, false)->GetUUID();
			case ScriptReferenceKind::AudioClip: return m_AssetManager.Load<AudioClip>(droppedPath, false)->GetUUID();
			case ScriptReferenceKind::Scene: return m_AssetManager.Load<Scene>(droppedPath, false)->GetUUID();
			default: return Constants::InvalidUUID;
			}
		}

		void RenderExposedScriptProperties(ScriptComponent& component)
		{
			if (component.ScriptHandle == Constants::InvalidUUID)
				return;

			auto scriptAsset = m_AssetManager.GetAsset<Script>(component.ScriptHandle);
			if (!scriptAsset)
				return;

			ImGui::Separator();

			ImGui::TextDisabled("Script Properties");
			ImGui::SameLine();
			float buttonWidth = ImGui::CalcTextSize("Refresh").x + ImGui::GetStyle().FramePadding.x * 2.0f;
			ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - buttonWidth);
			if (ImGui::SmallButton("Refresh"))
			{
				RefreshScriptProperties(component, scriptAsset);
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Re-parse the script file to pick up added/removed exposed properties.");

			auto& defaultProperties = scriptAsset->GetExposedProperties(); // The ones parsed from the .lua file
			if (defaultProperties.empty())
				return;

			ImGui::PushID(scriptAsset->GetName().c_str());
			if (UI::PropertyGrid::Begin("Exposed Properties"))
			{
				for (const auto& defaultProp : defaultProperties)
				{
					bool hasOverride = component.UserPropertyOverrides.find(defaultProp.Name) != component.UserPropertyOverrides.end();
					ScriptProperty activeProp = hasOverride ? component.UserPropertyOverrides[defaultProp.Name] : defaultProp;

					switch (activeProp.Type)
					{
					case ScriptPropertyType::Bool:
					{
						bool val = std::get<bool>(activeProp.Value);
						if (UI::PropertyGrid::Checkbox(activeProp.Name, val))
							ScriptEngine::SetScriptPropertyOverride<bool>(component, activeProp.Name, val);
						break;
					}
					case ScriptPropertyType::Int:
					{
						int val = std::get<int>(activeProp.Value);
						if (UI::PropertyGrid::Int(activeProp.Name, val))
							ScriptEngine::SetScriptPropertyOverride<int>(component, activeProp.Name, val);
						break;
					}
					case ScriptPropertyType::Float:
					{
						float val = std::get<float>(activeProp.Value);
						if (UI::PropertyGrid::Float(activeProp.Name, val))
							ScriptEngine::SetScriptPropertyOverride<float>(component, activeProp.Name, val);
						break;
					}
					case ScriptPropertyType::String:
					{
						std::string val = std::get<std::string>(activeProp.Value);
						if (UI::PropertyGrid::InputText(activeProp.Name, val))
							ScriptEngine::SetScriptPropertyOverride<std::string>(component, activeProp.Name, val);
						break;
					}
					case ScriptPropertyType::Vector3f:
					{
						Vector3f val = std::get<Vector3f>(activeProp.Value);
						if (UI::PropertyGrid::Float3(activeProp.Name, val))
							ScriptEngine::SetScriptPropertyOverride<Vector3f>(component, activeProp.Name, val);
						break;
					}
					case ScriptPropertyType::EntityRef:
					{
						UUID currentUUID = std::get<UUID>(activeProp.Value);
						UUID droppedUUID = Constants::InvalidUUID;
						std::string payloadType = DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::SceneEntity);
						UI::UICallbackFunc clearFunc = currentUUID != Constants::InvalidUUID ? UI::UICallbackFunc([&]() {
							ScriptEngine::SetScriptReferencePropertyOverride(component, activeProp.Name, Constants::InvalidUUID, ScriptPropertyType::EntityRef, ScriptReferenceKind::Entity);
							component.Initialized = false;
						}) : nullptr;

						if (UI::PropertyGrid::EntityReference(activeProp.Name, ResolveEntityReferenceName(currentUUID), payloadType, droppedUUID, clearFunc))
						{
							ScriptEngine::SetScriptReferencePropertyOverride(component, activeProp.Name, droppedUUID, ScriptPropertyType::EntityRef, ScriptReferenceKind::Entity);
							component.Initialized = false;
						}
						break;
					}
					case ScriptPropertyType::AssetRef:
					{
						ScriptReferenceKind referenceKind = defaultProp.ReferenceKind != ScriptReferenceKind::None ? defaultProp.ReferenceKind : activeProp.ReferenceKind;
						UUID currentUUID = std::get<UUID>(activeProp.Value);
						std::string droppedPath;
						std::string payloadType = ReferencePayloadType(referenceKind);
						UI::UICallbackFunc clearFunc = currentUUID != Constants::InvalidUUID ? UI::UICallbackFunc([&]() {
							ScriptEngine::SetScriptReferencePropertyOverride(component, activeProp.Name, Constants::InvalidUUID, ScriptPropertyType::AssetRef, referenceKind);
							component.Initialized = false;
						}) : nullptr;

						if (!payloadType.empty() && UI::PropertyGrid::AssetReference(activeProp.Name, ResolveAssetReferenceName(currentUUID, referenceKind), payloadType, droppedPath, nullptr, clearFunc))
						{
							UUID assetUUID = LoadDroppedReferenceAsset(referenceKind, droppedPath);
							ScriptEngine::SetScriptReferencePropertyOverride(component, activeProp.Name, assetUUID, ScriptPropertyType::AssetRef, referenceKind);
							component.Initialized = false;
						}
						break;
					}
					case ScriptPropertyType::Enum:
					{
						// Use the parsed default's options so we always have the full option list,
						// even if the override only stores the selected int value.
						const auto& options = defaultProp.EnumOptions;
						int currentVal = std::get<int>(activeProp.Value);

						const char* previewLabel = "";
						for (const auto& [optName, optValue] : options)
						{
							if (optValue == currentVal)
							{
								previewLabel = optName.c_str();
								break;
							}
						}

						if (UI::PropertyGrid::BeginComboBox(activeProp.Name, previewLabel))
						{
							for (const auto& [optName, optValue] : options)
							{
								bool isSelected = (optValue == currentVal);
								if (UI::PropertyGrid::ComboBoxItem(optName, isSelected))
								{
									ScriptEngine::SetScriptEnumPropertyOverride(component, activeProp.Name, optValue, options);
								}
							}
							UI::PropertyGrid::EndComboBox();
						}
						break;
					}
					default:
						EB_CORE_WARN("Unsupported script property type for '{}'", activeProp.Name);
						break;
					}
				}

				UI::PropertyGrid::End();
			}
			
			ImGui::PopID();
		}
	};
}