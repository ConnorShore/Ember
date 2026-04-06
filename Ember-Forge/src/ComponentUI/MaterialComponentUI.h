#pragma once
#include "ComponentUI.h"
#include "UI/UIWidgets.h"

#include <Ember/Event/UIEvent.h>
#include <variant>

namespace Ember {

	class MaterialComponentUI : public ComponentUI<MaterialComponent>
	{
	public:
		MaterialComponentUI(EditorContext* context) : ComponentUI(context) {}
		inline const char* GetName() const override { return "Material Component"; }

		virtual void CreateComponentForEntity(Entity entity) override
		{
			MaterialComponent comp{ Constants::Assets::StandardGeometryMatUUID };
			auto ret = comp.GetInstanced(entity.GetName() + "_Material");
			m_Context->ActiveScene->AttachComponent(entity, comp);
		}

	protected:
		inline void RenderComponentImpl(MaterialComponent& component) override
		{
			if (component.MaterialHandle == Constants::InvalidUUID)
				return;

			auto material = Application::Instance().GetAssetManager().GetAsset<MaterialBase>(component.MaterialHandle);
			ImGui::Text("Material: %s", material->GetName().c_str());
			ImGui::Separator();

			std::string currentMaterialName = material ? material->GetName() : "None";
			if (UI::BeginComboBox("##MaterialCombo", currentMaterialName.c_str()))
			{
				auto materials = m_Context->ActiveScene->GetAssetsOfType<MaterialBase>();
				for (auto& mat : materials)
				{
					// Check if this is the currently active shader
					bool isSelected = material && (material->GetUUID() == mat->GetUUID());
					if (UI::ComboBoxItem(mat->GetName().c_str(), isSelected))
					{
						std::string name = mat->GetName() + "_" + m_Context->SelectedEntity.GetComponent<TagComponent>().Tag;
						component.MaterialHandle = mat->GetUUID();
						component.GetInstanced(name);
					}

					// Set the initial focus when opening the combo
					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}
				UI::EndComboBox();
			}
			ImGui::SameLine();
			if (ImGui::Button("Clone"))
			{
				std::string entityName = m_Context->SelectedEntity.GetComponent<TagComponent>().Tag;
				std::string cloneName = entityName + "_" + material->GetName() + "_Clone";

				auto clonedMaterial = component.CloneMaterial(cloneName);
				if (clonedMaterial)
					m_Context->ActiveScene->RegisterAsset(clonedMaterial);
			}

			ImGui::Text("Shader: %s", material->GetShader()->GetName().c_str());
			ImGui::SameLine();

			if (ImGui::Button("Edit"))
			{
				if (material->GetShader())
				{
					EB_CORE_TRACE("Opening shader file: {}", material->GetShader()->GetFilePath());
					std::string shaderPath = material->GetShader()->GetFilePath();
					std::string command = "code " + shaderPath;
					system(command.c_str());
				}
			}

			ImGui::Separator();

			// Render property widgets driven by @UIProperty annotations in the shader
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
	};

}