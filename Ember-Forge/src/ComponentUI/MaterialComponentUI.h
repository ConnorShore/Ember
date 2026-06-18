#pragma once
#include "ComponentUI.h"
#include "UI/UIWidgets.h"

#include <Ember/Event/UIEvent.h>
#include <Ember/Asset/MaterialSerializer.h>
#include <Ember/Utils/PlatformUtil.h>
#include <Ember/Core/ProjectManager.h>

#include <variant>
#include <format>
#include <fstream>
#include <filesystem>

namespace Ember {

	class MaterialComponentUI : public ComponentUI<MaterialComponent>
	{
	public:
		MaterialComponentUI(EditorContext* context) : ComponentUI(context) {}
		inline const char* GetName() const override { return "Material Component"; }

		virtual void CreateComponentForEntity(Entity entity) override
		{
			auto& materialComponent = entity.AttachComponent<MaterialComponent>();
			materialComponent.MaterialHandle = Constants::InvalidUUID;
		}

	protected:
		inline void RenderComponentImpl(MaterialComponent& component) override
		{
			bool containsMaterial = component.MaterialHandle != Constants::InvalidUUID;
			SharedPtr<MaterialBase> material = nullptr;

			if (containsMaterial)
			{
				material = m_AssetManager.GetAsset<MaterialBase>(component.MaterialHandle);
			}

			// 1. Material Selection & Creation
			DrawMaterialHeader(component, material, containsMaterial);

			if (containsMaterial && material)
			{
				// 2. Material Actions (Clone, Rename)
				DrawMaterialActions(component, material);

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				// 3. Shader Selection & Config
				DrawShaderSection(material);

				// 4. Exposed Shader Properties
				if (material->GetShader())
				{
					DrawMaterialProperties(material);
				}
			}

			// 5. Render active popups/modals
			DrawRenameModal(material);
			DrawCreateShaderModal(material);
		}

	private:
		// ==============================================================================
		// UI RENDERING METHODS
		// ==============================================================================

		void DrawMaterialHeader(MaterialComponent& component, const SharedPtr<MaterialBase>& material, bool containsMaterial)
		{
			std::string materialName = material ? material->GetName() : "None";

			// 1. Clearly display the active material name with a slight color tint
			ImGui::Text("Active Material: ");
			ImGui::SameLine();
			if (containsMaterial)
				ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "%s", materialName.c_str()); // Greenish text
			else
				ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "None"); // Reddish text

			ImGui::Spacing();

			// 2. Full-width Dropdown to select existing materials
			ImGui::PushItemWidth(-1);
			if (UI::BeginComboBox("##MaterialCombo", materialName.c_str()))
			{
				auto materials = m_Context->ActiveScene()->GetAssetsOfType<MaterialBase>();

				if (UI::ComboBoxItem("None", !containsMaterial))
					component.MaterialHandle = Constants::InvalidUUID;

				ImGui::Separator();

				for (auto& mat : materials)
				{
					bool isSelected = material && (material->GetUUID() == mat->GetUUID());
					if (UI::ComboBoxItem(mat->GetName().c_str(), isSelected))
					{
						component.MaterialHandle = mat->GetUUID();
					}

					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}
				UI::EndComboBox();
			}
			ImGui::PopItemWidth();

			// 3. Only show "Create" if nothing is selected
			if (!containsMaterial)
			{
				ImGui::Spacing();
				if (ImGui::Button("Create New Material", ImVec2(-1, 0))) // Full width button
				{
					CreateNewMaterial(component);
				}
			}
		}

		void DrawMaterialActions(MaterialComponent& component, SharedPtr<MaterialBase>& material)
		{
			// Render a clean horizontal action bar
			float buttonWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

			if (ImGui::Button("Clone", ImVec2(buttonWidth, 0)))
			{
				CloneMaterial(component, material);
			}

			ImGui::SameLine();

			if (ImGui::Button("Rename", ImVec2(buttonWidth, 0)))
			{
				m_OpenRenamePopup = true;
			}
		}

		void DrawShaderSection(SharedPtr<MaterialBase>& material)
		{
			ImGui::TextDisabled("SHADER SETTINGS");
			ImGui::Spacing();

			if (UI::PropertyGrid::Begin("ShaderConfig"))
			{
				// --- Shader Selection ---
				auto addShaderFunc = [&]() { m_OpenCreateShaderPopup = true; };
				auto clearShaderFunc = [&]() { material->SetShader(nullptr); };

				std::map<std::string, std::vector<SharedPtr<Shader>>> shaderMap;
				auto shaders = m_AssetManager.GetAssetsOfType<Shader>();
				for (auto& shader : shaders)
				{
					if (shader->IsEngineAsset()) 
						shaderMap["Preset"].push_back(shader);
					else 
						shaderMap["Custom"].push_back(shader);
				}

				std::string defaultVal = material->GetShader() ? material->GetShader()->GetName() : "None";
				if (UI::PropertyGrid::ComboBoxWithActions("Shader", defaultVal, addShaderFunc, clearShaderFunc))
				{
					if (UI::ComboBoxItem("None", !material->GetShader()))
						material->SetShader(nullptr);

					ImGui::Separator();

					ImGui::TextDisabled("Presets");
					ImGui::Indent();
					for (auto& shader : shaderMap["Preset"])
					{
						bool isSelected = material->GetShader() && (material->GetShader()->GetUUID() == shader->GetUUID());
						if (UI::ComboBoxItem(shader->GetName().c_str(), isSelected))
							material->SetShader(shader);
						if (isSelected) ImGui::SetItemDefaultFocus();
					}
					ImGui::Unindent();

					ImGui::Separator();

					ImGui::TextDisabled("Custom");
					ImGui::Indent();
					for (auto& shader : shaderMap["Custom"])
					{
						bool isSelected = material->GetShader() && (material->GetShader()->GetUUID() == shader->GetUUID());
						if (UI::ComboBoxItem(shader->GetName().c_str(), isSelected))
							material->SetShader(shader);
						if (isSelected) ImGui::SetItemDefaultFocus();
					}
					ImGui::Unindent();

					UI::EndComboBox();
				}

				// --- Shader Configurations (Render Queue & Edit) ---
				if (!material->IsEngineAsset() && material->GetShader() && !material->GetShader()->IsEngineAsset())
				{
					if (UI::PropertyGrid::BeginComboBox("Render Queue", RenderQueueToString(material->GetRenderQueue()).c_str()))
					{
						auto renderQueues = { RenderQueue::Opaque, RenderQueue::Forward, RenderQueue::Transparent };
						for (const auto& rq : renderQueues)
						{
							bool isSelected = material->GetRenderQueue() == rq;
							if (UI::ComboBoxItem(RenderQueueToString(rq).c_str(), isSelected))
								material->SetRenderQueue(rq);
							if (isSelected) ImGui::SetItemDefaultFocus();
						}
						UI::EndComboBox();
					}
				}

				UI::PropertyGrid::End();
			}

			if (!material->IsEngineAsset() && material->GetShader() && !material->GetShader()->IsEngineAsset())
			{
				ImGui::Text("Source Code");
				ImGui::NextColumn();
				if (ImGui::Button("Open in VS Code", ImVec2(-1, 0)))
				{
					EB_CORE_TRACE("Opening shader file: {}", material->GetShader()->GetFilePath());
					std::string command = "code " + material->GetShader()->GetFilePath();
					system(command.c_str());
				}
			}
		}

		void DrawMaterialProperties(SharedPtr<MaterialBase>& material)
		{
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();
			ImGui::TextDisabled("MATERIAL PROPERTIES");
			ImGui::Spacing();

			if (material->IsEngineAsset())
			{
				ImGui::BeginDisabled(true);
				ImGui::TextWrapped("This is an engine material. To edit its properties, create a clone by clicking the 'Clone' button above.");
				ImGui::EndDisabled();
				ImGui::Separator();
				return;
			}

			// Seed defaults for any newly-introduced shader uniforms 
			EnsureMaterialUniformsForShader(material);

			if (UI::PropertyGrid::Begin("MaterialProps"))
			{
				auto& shaderProps = material->GetShader()->GetProperties();
				for (auto& prop : shaderProps)
				{
					switch (prop.Type)
					{
					case ShaderPropertyType::Float:
						RenderProperty<float>(prop, material, [&prop](const std::string& name, float* value) {
							return UI::PropertyGrid::Float(name, *value, prop.Step, prop.Min, prop.Max);
							});
						break;
					case ShaderPropertyType::Float2:
						RenderProperty<Vector2f>(prop, material, [&prop](const std::string& name, Vector2f* value) {
							return UI::PropertyGrid::Float2(name, *value, prop.Step, prop.Min, prop.Max);
							});
						break;
					case ShaderPropertyType::Float3:
						RenderProperty<Vector3f>(prop, material, [&prop](const std::string& name, Vector3f* value) {
							return UI::PropertyGrid::Float3(name, *value, prop.Step, prop.Min, prop.Max);
							});
						break;
					case ShaderPropertyType::Float4:
						RenderProperty<Vector4f>(prop, material, [&prop](const std::string& name, Vector4f* value) {
							return UI::PropertyGrid::Float4(name, *value, prop.Step, prop.Min, prop.Max);
							});
						break;
					case ShaderPropertyType::Color3:
						RenderProperty<Vector3f>(prop, material, [](const std::string& name, Vector3f* value) {
							return UI::PropertyGrid::Color3(name, *value);
							});
						break;
					case ShaderPropertyType::Color4:
						RenderProperty<Vector4f>(prop, material, [](const std::string& name, Vector4f* value) {
							return UI::PropertyGrid::Color4(name, *value);
							});
						break;
					case ShaderPropertyType::Slider:
						RenderProperty<float>(prop, material, [&prop](const std::string& name, float* value) {
							return UI::PropertyGrid::SliderFloat(name, *value, prop.Min, prop.Max);
							});
						break;
					case ShaderPropertyType::Texture:
					{
						SharedPtr<Texture2D> currentTexture = nullptr;
						bool hasTexture = material->ContainsUniform(prop.UniformName);
						if (hasTexture)
							currentTexture = std::get<SharedPtr<Texture2D>>(material->GetUniforms().at(prop.UniformName));

						bool hasValidTexture = currentTexture
							&& currentTexture->GetName() != Constants::Assets::DefaultWhiteTex
							&& currentTexture->GetName() != Constants::Assets::DefaultNormalTex
							&& currentTexture->GetName() != Constants::Assets::DefaultErrorTex;
						UUID texUUID = hasValidTexture ? currentTexture->GetUUID() : UUID(Constants::InvalidUUID);

						std::string droppedFilePath;
						if (UI::PropertyGrid::DragDropTexture(prop.DisplayName, texUUID, droppedFilePath, [&]() {
							auto defaultTex = GetDefaultTextureForUniform(prop.UniformName);
							material->SetUniform(prop.UniformName, defaultTex);
							}))
						{
							auto newTexture = m_AssetManager.Load<Texture2D>(droppedFilePath);
							material->SetUniform(prop.UniformName, newTexture);

							auto evt = UINotificationEvent(std::format("{} texture updated to {}", prop.UniformName, droppedFilePath));
							m_Context->EventCallback(evt);
						}
						break;
					}
					}
				}
				UI::PropertyGrid::End();
			}
		}

		// ==============================================================================
		// MODALS & POPUPS
		// ==============================================================================

		void DrawRenameModal(SharedPtr<MaterialBase>& material)
		{
			if (m_OpenRenamePopup && material)
			{
				ImGui::OpenPopup("Rename Material");
				m_OpenRenamePopup = false;
			}

			if (ImGui::BeginPopupModal("Rename Material", NULL, ImGuiWindowFlags_AlwaysAutoResize))
			{
				static char materialName[128] = {};
				if (ImGui::IsWindowAppearing())
					strncpy(materialName, material->GetName().c_str(), sizeof(materialName) - 1);

				ImGui::InputText("Material Name", materialName, sizeof(materialName));
				ImGui::Spacing();

				if (ImGui::Button("OK", ImVec2(120, 0)))
				{
					material->Rename(materialName);
					ImGui::CloseCurrentPopup();
				}

				ImGui::SameLine();

				if (ImGui::Button("Cancel", ImVec2(120, 0)))
				{
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}
		}

		void DrawCreateShaderModal(SharedPtr<MaterialBase>& material)
		{
			if (m_OpenCreateShaderPopup)
			{
				ImGui::OpenPopup("Create New Shader");
				m_OpenCreateShaderPopup = false;
			}

			if (ImGui::BeginPopupModal("Create New Shader", NULL, ImGuiWindowFlags_AlwaysAutoResize))
			{
				static char shaderName[128] = "NewShader";
				static RenderQueue selectedQueue = RenderQueue::Opaque;

				ImGui::InputText("Shader Name", shaderName, sizeof(shaderName));

				if (ImGui::BeginCombo("Render Queue", RenderQueueToString(selectedQueue).c_str()))
				{
					auto renderQueues = { RenderQueue::Opaque, RenderQueue::Forward, RenderQueue::Transparent };
					for (const auto& rq : renderQueues)
					{
						bool isSelected = selectedQueue == rq;
						if (ImGui::Selectable(RenderQueueToString(rq).c_str(), isSelected))
							selectedQueue = rq;
						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}

				ImGui::Spacing();

				auto shaderDir = ProjectManager::GetActive()->GetDefaultDirectoryForAsset(AssetType::Shader);

				if (ImGui::Button("Create", ImVec2(120, 0)))
				{
					std::filesystem::path newShaderPath = shaderDir / std::format("{}.glsl", shaderName);
					if (std::filesystem::exists(std::filesystem::absolute(newShaderPath)))
					{
						ImGui::OpenPopup("Shader Exists");
					}
					else
					{
						GenerateShaderPresetScript(shaderName, newShaderPath, material, selectedQueue);
						strcpy_s(shaderName, "NewShader");
						selectedQueue = RenderQueue::Opaque;
						ImGui::CloseCurrentPopup();
					}
				}

				ImGui::SameLine();

				if (ImGui::Button("Cancel", ImVec2(120, 0)))
				{
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::BeginPopupModal("Shader Exists", NULL, ImGuiWindowFlags_AlwaysAutoResize))
				{
					ImGui::Text("A shader with that name already exists.\nPlease choose a different name.");
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

		// ==============================================================================
		// ACTIONS & LOGIC HELPER METHODS
		// ==============================================================================

		void CreateNewMaterial(MaterialComponent& component)
		{
			std::string entityName = m_Context->SelectedEntity.GetComponent<TagComponent>().Tag;
			std::string newMaterialName = entityName + "_Material";
			auto newMaterial = m_AssetManager.Create<Material>(newMaterialName, nullptr, RenderQueue::Opaque);

			if (newMaterial)
			{
				std::filesystem::path assetDirectory = ProjectManager::GetActive()->GetAssetDirectory();
				std::filesystem::path filePath = assetDirectory / "Materials" / (newMaterialName + ".ebmat");
				newMaterial->SetFilePath(filePath.string());
				newMaterial->SetIsEngineAsset(false);

				if (!MaterialSerializer::Serialize(filePath, newMaterial))
					EB_CORE_ERROR("Failed to serialize new material!");

				m_Context->ActiveScene()->RegisterAsset(newMaterial);
				component.MaterialHandle = newMaterial->GetUUID();
			}
		}

		void CloneMaterial(MaterialComponent& component, SharedPtr<MaterialBase>& material)
		{
			std::string entityName = m_Context->SelectedEntity.GetComponent<TagComponent>().Tag;
			std::string cloneName = entityName + "_" + material->GetName() + "_Clone";

			auto clonedMaterial = component.CloneMaterial(cloneName);
			if (clonedMaterial)
			{
				std::filesystem::path assetDirectory = ProjectManager::GetActive()->GetAssetDirectory();
				std::filesystem::path filePath = assetDirectory / "Materials" / (cloneName + ".ebmat");
				clonedMaterial->SetFilePath(filePath.string());

				if (!MaterialSerializer::Serialize(filePath, clonedMaterial))
					EB_CORE_ERROR("Failed to serialize cloned material!");

				m_Context->ActiveScene()->RegisterAsset(clonedMaterial);
			}
		}

		template<typename T, typename RenderFunc>
		void RenderProperty(const ShaderProperty& prop, const SharedPtr<MaterialBase>& material, RenderFunc renderFunc)
		{
			if (!material->ContainsUniform(prop.UniformName))
				return;

			T value = std::get<T>(material->GetUniforms().at(prop.UniformName));
			if (renderFunc(prop.UniformName, &value))
			{
				if (prop.Normalize)
					value = Math::Normalize<T>(value, prop.Min, prop.Max);

				material->SetUniform(prop.UniformName, value);
			}
		}

		void EnsureMaterialUniformsForShader(const SharedPtr<MaterialBase>& material)
		{
			if (!material || !material->GetShader()) return;

			for (const auto& prop : material->GetShader()->GetProperties())
			{
				if (material->ContainsUniform(prop.UniformName)) continue;

				switch (prop.Type)
				{
				case ShaderPropertyType::Float:		material->SetUniform(prop.UniformName, 0.0f); break;
				case ShaderPropertyType::Slider:	material->SetUniform(prop.UniformName, (prop.Min + prop.Max) * 0.5f); break;
				case ShaderPropertyType::Float2:	material->SetUniform(prop.UniformName, Vector2f(0.0f)); break;
				case ShaderPropertyType::Float3:	material->SetUniform(prop.UniformName, Vector3f(0.0f)); break;
				case ShaderPropertyType::Float4:	material->SetUniform(prop.UniformName, Vector4f(0.0f)); break;
				case ShaderPropertyType::Color3:	material->SetUniform(prop.UniformName, Vector3f(1.0f)); break;
				case ShaderPropertyType::Color4:	material->SetUniform(prop.UniformName, Vector4f(1.0f)); break;
				case ShaderPropertyType::Texture:	material->SetUniform(prop.UniformName, GetDefaultTextureForUniform(prop.UniformName)); break;
				}
			}
		}

		SharedPtr<Texture2D> GetDefaultTextureForUniform(const std::string& uniformName)
		{
			std::string nameLower = uniformName;
			std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);

			if (nameLower.find("normal") != std::string::npos || nameLower.find("bump") != std::string::npos)
				return m_AssetManager.GetAsset<Texture2D>(Constants::Assets::DefaultNormalTex);
			else if (nameLower.find("emiss") != std::string::npos || nameLower.find("ao") != std::string::npos)
				return m_AssetManager.GetAsset<Texture2D>(Constants::Assets::DefaultBlackTex);
			else
				return m_AssetManager.GetAsset<Texture2D>(Constants::Assets::DefaultWhiteTex);
		}

		// ==============================================================================
		// SHADER GENERATION METHODS
		// ==============================================================================

		void WriteSharedVertexStage(std::ofstream& out)
		{
			out << "#shader vertex\n";
			out << "#version 450 core\n\n";
			out << "layout(location = 0) in vec3 v_Position;\n";
			out << "layout(location = 1) in vec3 v_Normal;\n";
			out << "layout(location = 2) in vec2 v_TextureCoord;\n\n";
			out << "layout(std140, binding = 0) uniform CameraData\n";
			out << "{\n";
			out << "    mat4 u_ViewProjection;\n";
			out << "};\n\n";
			out << "uniform mat4 u_Transform;\n\n";
			out << "out vec2 TextureCoord;\n";
			out << "out vec3 WorldNormal;\n";
			out << "out vec3 WorldPosition;\n\n";
			out << "void main()\n";
			out << "{\n";
			out << "    TextureCoord = v_TextureCoord;\n";
			out << "    WorldNormal = mat3(u_Transform) * v_Normal;\n";
			out << "    vec4 worldPos = u_Transform * vec4(v_Position, 1.0);\n";
			out << "    WorldPosition = worldPos.xyz;\n";
			out << "    gl_Position = u_ViewProjection * worldPos;\n";
			out << "}\n\n";
		}

		void GenerateShaderPresetScript(const std::string& shaderName, const std::filesystem::path& filePath, SharedPtr<MaterialBase>& material, RenderQueue queue)
		{
			std::filesystem::create_directories(filePath.parent_path());

			std::ofstream out(filePath);
			WriteSharedVertexStage(out);

			out << "#shader fragment\n";
			out << "#version 450 core\n\n";

			switch (queue)
			{
			case RenderQueue::Opaque:
			{
				out << "layout(location = 0) out vec4 AlbedoRoughness;\n";
				out << "layout(location = 1) out vec4 NormalMetallic;\n";
				out << "layout(location = 2) out vec4 PositionAO;\n";
				out << "layout(location = 3) out vec4 EmissionOut;\n";
				out << "layout(location = 4) out int EntityID;\n\n";
				out << "in vec2 TextureCoord;\n";
				out << "in vec3 WorldNormal;\n";
				out << "in vec3 WorldPosition;\n\n";
				out << "// @UIProperty(Name = \"Albedo\", Type = Color3)\n";
				out << "uniform vec3 u_Albedo;\n";
				out << "// @UIProperty(Name = \"Roughness\", Type = Slider, Min = 0.0, Max = 1.0)\n";
				out << "uniform float u_Roughness;\n";
				out << "// @UIProperty(Name = \"Metallic\", Type = Slider, Min = 0.0, Max = 1.0)\n";
				out << "uniform float u_Metallic;\n\n";
				out << "uniform int u_EntityID;\n\n";
				out << "void main()\n";
				out << "{\n";
				out << "    AlbedoRoughness = vec4(u_Albedo, u_Roughness);\n";
				out << "    NormalMetallic = vec4(normalize(WorldNormal), u_Metallic);\n";
				out << "    PositionAO = vec4(WorldPosition, 1.0);\n";
				out << "    EmissionOut = vec4(0.0);\n";
				out << "    EntityID = u_EntityID;\n";
				out << "}\n";
				break;
			}
			case RenderQueue::Forward:
			case RenderQueue::Transparent:
			{
				const bool isTransparent = (queue == RenderQueue::Transparent);
				out << "layout(location = 0) out vec4 OutColor;\n";
				out << "layout(location = 1) out vec4 BrightColor;\n";
				out << "layout(location = 2) out int EntityID;\n\n";
				out << "in vec2 TextureCoord;\n";
				out << "in vec3 WorldNormal;\n";
				out << "in vec3 WorldPosition;\n\n";
				out << "// @UIProperty(Name = \"Color\", Type = Color3)\n";
				out << "uniform vec3 u_Color;\n";
				if (isTransparent)
				{
					out << "// @UIProperty(Name = \"Opacity\", Type = Slider, Min = 0.0, Max = 1.0)\n";
					out << "uniform float u_Opacity;\n";
				}
				out << "// @UIProperty(Name = \"Emission Intensity\", Type = Float, Min = 0.0, Max = 50.0, Step = 0.05)\n";
				out << "uniform float u_Emission;\n\n";
				out << "uniform int u_EntityID;\n\n";
				out << "void main()\n";
				out << "{\n";
				out << "    vec3 finalColor = u_Color * u_Emission;\n";
				if (isTransparent)
					out << "    OutColor = vec4(finalColor, u_Opacity);\n";
				else
					out << "    OutColor = vec4(finalColor, 1.0);\n";
				out << "    BrightColor = vec4(max(OutColor.rgb - vec3(1.0), vec3(0.0)), 1.0);\n";
				out << "    EntityID = u_EntityID;\n";
				out << "}\n";
				break;
			}
			default: break;
			}
			out.close();

			auto shaderAsset = m_AssetManager.Load<Shader>(shaderName, filePath.string());
			if (shaderAsset)
			{
				shaderAsset->SetIsEngineAsset(false);
				m_Context->ActiveScene()->RegisterAsset(shaderAsset);
				material->SetShader(shaderAsset);
				material->SetRenderQueue(queue);

				std::string command = "code \"" + filePath.string() + "\"";
				system(command.c_str());
			}
			else
			{
				EB_CORE_ERROR("Failed to load newly created shader: {}", filePath.string());
			}
		}

	private:
		bool m_OpenRenamePopup = false;
		bool m_OpenCreateShaderPopup = false;
	};

}