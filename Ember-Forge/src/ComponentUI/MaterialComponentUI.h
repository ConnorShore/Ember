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
			auto material = containsMaterial 
				? Application::Instance().GetAssetManager().GetAsset<MaterialBase>(component.MaterialHandle)
				: nullptr;

			std::string materialName = material ? material->GetName() : "None";
			ImGui::Text("Material: ", materialName.c_str());
			ImGui::Separator();

			if (UI::BeginComboBox("##MaterialCombo", materialName.c_str()))
			{
				auto materials = m_Context->ActiveScene()->GetAssetsOfType<MaterialBase>();
				if (UI::ComboBoxItem("None", !containsMaterial))
					component.MaterialHandle = Constants::InvalidUUID;

				ImGui::Separator();

				for (auto& mat : materials)
				{
					// Check if this is the currently active shader
					bool isSelected = material && (material->GetUUID() == mat->GetUUID());
					if (UI::ComboBoxItem(mat->GetName().c_str(), isSelected))
					{
						component.MaterialHandle = mat->GetUUID();
					}

					// Set the initial focus when opening the combo
					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}
				UI::EndComboBox();
			}

			ImGui::SameLine();

			if (!containsMaterial)
			{
				if (ImGui::Button("Create"))
				{
					std::string entityName = m_Context->SelectedEntity.GetComponent<TagComponent>().Tag;
					std::string newMaterialName = entityName + "_Material";
					auto newMaterial = Application::Instance().GetAssetManager().Create<Material>(newMaterialName, nullptr, RenderQueue::Opaque);
					if (newMaterial)
					{
						// Serialize the asset
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
			}
			else
			{
				RenderPopulatedMaterialWidgets(component, material);
			}
			
		}

	private:
		// Reads a uniform from the material variant, renders an ImGui widget,
		// and writes the value back if changed. Optional normalize maps [min,max] to [0,1].
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

		// Seeds the material with sensible defaults for any shader property whose uniform
		// isn't already set. Without this, a freshly-created (or hot-reloaded) shader's
		// properties don't appear in the UI (the per-property widget early-outs when the
		// uniform is missing), and the GPU reads uninitialized uniforms as zero -- which
		// renders forward shaders black and makes opaque G-buffer pixels look transparent.
		void EnsureMaterialUniformsForShader(const SharedPtr<MaterialBase>& material)
		{
			if (!material || !material->GetShader())
				return;

			for (const auto& prop : material->GetShader()->GetProperties())
			{
				if (material->ContainsUniform(prop.UniformName))
					continue;

				switch (prop.Type)
				{
				case ShaderPropertyType::Float:
					material->SetUniform(prop.UniformName, 0.0f);
					break;
				case ShaderPropertyType::Slider:
					// Midpoint of the slider range gives reasonable defaults like
					// Roughness=0.5 / Opacity=0.5 instead of zero.
					material->SetUniform(prop.UniformName, (prop.Min + prop.Max) * 0.5f);
					break;
				case ShaderPropertyType::Float2:
					material->SetUniform(prop.UniformName, Vector2f(0.0f));
					break;
				case ShaderPropertyType::Float3:
					material->SetUniform(prop.UniformName, Vector3f(0.0f));
					break;
				case ShaderPropertyType::Float4:
					material->SetUniform(prop.UniformName, Vector4f(0.0f));
					break;
				case ShaderPropertyType::Color3:
					material->SetUniform(prop.UniformName, Vector3f(1.0f));
					break;
				case ShaderPropertyType::Color4:
					material->SetUniform(prop.UniformName, Vector4f(1.0f));
					break;
				case ShaderPropertyType::Texture:
					material->SetUniform(prop.UniformName, GetDefaultTextureForUniform(prop.UniformName));
					break;
				}
			}
		}

		// Helper to determine default texture to apply
		SharedPtr<Texture2D> GetDefaultTextureForUniform(const std::string& uniformName)
		{
			auto& assetManager = Application::Instance().GetAssetManager();

			std::string nameLower = uniformName;
			std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);

			if (nameLower.find("normal") != std::string::npos || nameLower.find("bump") != std::string::npos)
			{
				return assetManager.GetAsset<Texture2D>(Constants::Assets::DefaultNormalTex);
			}
			else if (nameLower.find("emiss") != std::string::npos || nameLower.find("ao") != std::string::npos)
			{
				// Emission and AO use black texture
				return assetManager.GetAsset<Texture2D>(Constants::Assets::DefaultBlackTex);
			}
			else
			{
				// Albedo, Metallic, Roughness, and general masks default to White
				return assetManager.GetAsset<Texture2D>(Constants::Assets::DefaultWhiteTex);
			}
		}

		void RenderPopulatedMaterialWidgets(MaterialComponent component, SharedPtr<MaterialBase>& material)
		{
			auto& assetManager = Application::Instance().GetAssetManager();

			if (ImGui::Button("Clone"))
			{
				std::string entityName = m_Context->SelectedEntity.GetComponent<TagComponent>().Tag;
				std::string cloneName = entityName + "_" + material->GetName() + "_Clone";

				auto clonedMaterial = component.CloneMaterial(cloneName);
				if (clonedMaterial)
				{
					// Serialize the asset
					std::filesystem::path assetDirectory = ProjectManager::GetActive()->GetAssetDirectory();
					std::filesystem::path filePath = assetDirectory / "Materials" / (cloneName + ".ebmat");
					clonedMaterial->SetFilePath(filePath.string());

					if (!MaterialSerializer::Serialize(filePath, clonedMaterial))
						EB_CORE_ERROR("Failed to serialize cloned material!");

					m_Context->ActiveScene()->RegisterAsset(clonedMaterial);
				}
			}

			ImGui::SameLine();
			if (ImGui::Button("Rename"))
			{
				m_OpenRenamePopup = true;
			}
			OpenRenamePopup(material);

			std::string shaderName = material->GetShader() ? material->GetShader()->GetName() : "None";
			if (!material->GetShader())
			{
				if (UI::PropertyGrid::Begin("ShaderChooseProps"))
				{
					auto addShaderFunc = [&]() {
						m_OpenCreateShaderPopup = true;
					};

					auto clearShaderFunc = [&]() {
						material->SetShader(nullptr);
						};

					std::map<std::string, std::vector<SharedPtr<Shader>>> shaderMap;
					auto shaders = assetManager.GetAssetsOfType<Shader>();
					for (auto& shader : shaders)
					{
						if (shader->IsEngineAsset())
							shaderMap["Preset"].push_back(shader);
						else
							shaderMap["Custom"].push_back(shader);
					}

					if (UI::PropertyGrid::ComboBoxWithActions("Shader", addShaderFunc, clearShaderFunc))
					{
						if (UI::ComboBoxItem("None", !material->GetShader()))
							material->SetShader(nullptr);

						ImGui::Separator();

						// Preset shaders
						ImGui::TextDisabled("Presets");
						ImGui::Indent();
						for (auto& shader : shaderMap["Preset"])
						{
							bool isSelected = material->GetShader() && (material->GetShader()->GetUUID() == shader->GetUUID());
							if (UI::ComboBoxItem(shader->GetName().c_str(), isSelected))
							{
								material->SetShader(shader);
							}
							if (isSelected)
								ImGui::SetItemDefaultFocus();
						}
						ImGui::Unindent();

						ImGui::Separator();

						// Engine shaders
						ImGui::TextDisabled("Custom");
						ImGui::Indent();
						for (auto& shader : shaderMap["Custom"])
						{
							bool isSelected = material->GetShader() && (material->GetShader()->GetUUID() == shader->GetUUID());
							if (UI::ComboBoxItem(shader->GetName().c_str(), isSelected))
							{
								material->SetShader(shader);
							}
							if (isSelected)
								ImGui::SetItemDefaultFocus();
						}
						ImGui::Unindent();

						UI::EndComboBox();
					}

					UI::PropertyGrid::End();
				}

				DrawCreateShaderModal(material);
			}

			// Shader modification
			bool isEngineAsset = material->IsEngineAsset();
			if (!isEngineAsset && material->GetShader())
			{
				if (ImGui::Button("Edit"))
				{
					EB_CORE_TRACE("Opening shader file: {}", material->GetShader()->GetFilePath());
					std::string shaderPath = material->GetShader()->GetFilePath();
					std::string command = "code " + shaderPath;
					system(command.c_str());
				}

				if (UI::BeginComboBox("##RenderQueueCombo", RenderQueueToString(material->GetRenderQueue()).c_str()))
				{
					// Convert RenderQueue enum to array and iterate to avoid hardcoding options in the UI
					auto renderQueues = { RenderQueue::Opaque, RenderQueue::Forward, RenderQueue::Transparent };
					for (const auto& rq : renderQueues)
					{
						bool isSelected = material->GetRenderQueue() == rq;
						if (UI::ComboBoxItem(RenderQueueToString(rq).c_str(), isSelected))
						{
							material->SetRenderQueue(rq);
						}
						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}
					UI::EndComboBox();
				}

				ImGui::Separator();

				// Seed defaults for any newly-introduced shader uniforms (e.g. immediately
				// after the shader is assigned, or after a hot-reload that added a property)
				// so the property widgets render and the GPU doesn't read zeros.
				EnsureMaterialUniformsForShader(material);

				if (UI::PropertyGrid::Begin("MaterialProps"))
				{
					auto& shaderProps = material->GetShader()->GetProperties();
					for (auto& prop : shaderProps)
					{
						switch (prop.Type)
						{
						case ShaderPropertyType::Float:
						{
							RenderProperty<float>(prop, material, [&prop](const std::string& name, float* value) {
								return UI::PropertyGrid::Float(name, *value, prop.Step, prop.Min, prop.Max);
								});
							break;
						}
						case ShaderPropertyType::Float2:
						{
							RenderProperty<Vector2f>(prop, material, [&prop](const std::string& name, Vector2f* value) {
								return UI::PropertyGrid::Float2(name, *value, prop.Step, prop.Min, prop.Max);
								});
							break;
						}
						case ShaderPropertyType::Float3:
						{
							RenderProperty<Vector3f>(prop, material, [&prop](const std::string& name, Vector3f* value) {
								return UI::PropertyGrid::Float3(name, *value, prop.Step, prop.Min, prop.Max);
								});
							break;
						}
						case ShaderPropertyType::Float4:
						{
							RenderProperty<Vector4f>(prop, material, [&prop](const std::string& name, Vector4f* value) {
								return UI::PropertyGrid::Float4(name, *value, prop.Step, prop.Min, prop.Max);
								});
							break;
						}
						case ShaderPropertyType::Color3:
						{
							RenderProperty<Vector3f>(prop, material, [](const std::string& name, Vector3f* value) {
								return UI::PropertyGrid::Color3(name, *value);
								});
							break;
						}
						case ShaderPropertyType::Color4:
						{
							RenderProperty<Vector4f>(prop, material, [](const std::string& name, Vector4f* value) {
								return UI::PropertyGrid::Color4(name, *value);
								});
							break;
						}
						case ShaderPropertyType::Slider:
						{
							RenderProperty<float>(prop, material, [&prop](const std::string& name, float* value) {
								return UI::PropertyGrid::SliderFloat(name, *value, prop.Min, prop.Max);
								});
							break;
						}
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
								auto newTexture = Application::Instance().GetAssetManager().Load<Texture2D>(droppedFilePath);
								material->SetUniform(prop.UniformName, newTexture);

								// Add UI notification for the new texture
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
		}

		void OpenRenamePopup(SharedPtr<MaterialBase>& material)
		{
			if (m_OpenRenamePopup)
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

				// Render queue selector — the preset script is tailored to the chosen queue
				// (opaque writes to the G-buffer, forward/transparent output color directly with blending).
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

				auto shaderDir = ProjectManager::GetActive()->GetAssetDirectory() / "Shaders";

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

		// Writes the shared vertex stage (same for every queue) — a basic transformed mesh that
		// forwards UVs/normals/world position so the fragment stage has what it needs.
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
			// Ensure the Shaders directory exists
			std::filesystem::create_directories(filePath.parent_path());

			std::ofstream out(filePath);
			WriteSharedVertexStage(out);

			out << "#shader fragment\n";
			out << "#version 450 core\n\n";

			switch (queue)
			{
			case RenderQueue::Opaque:
			{
				// Opaque queue feeds the deferred G-buffer (albedo/roughness, normal/metallic,
				// position/AO, emission). Lighting is applied later by the engine's lighting pass.
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
				// Forward and transparent both shade directly to the lit color target.
				// Transparent additionally writes alpha for blending.
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
			default:
				break;
			}
			out.close();

			// Load it through the asset manager and register with the active scene
			auto& assetManager = Application::Instance().GetAssetManager();
			auto shaderAsset = assetManager.Load<Shader>(shaderName, filePath.string());
			if (shaderAsset)
			{
				shaderAsset->SetIsEngineAsset(false);
				m_Context->ActiveScene()->RegisterAsset(shaderAsset);
				material->SetShader(shaderAsset);
				material->SetRenderQueue(queue);

				// Open the new shader file in VS Code (matches existing "Edit" behavior)
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