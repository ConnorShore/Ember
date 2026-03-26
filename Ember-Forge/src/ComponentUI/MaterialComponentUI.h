#pragma once
#include "ComponentUI.h"
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
			if (ImGui::BeginCombo("##MaterialCombo", currentMaterialName.c_str()))
			{
				auto materials = m_Context->ActiveScene->GetAssetsOfType<MaterialBase>();
				for (auto& mat : materials)
				{
					// Check if this is the currently active shader
					bool isSelected = material && (material->GetUUID() == mat->GetUUID());
					if (ImGui::Selectable(mat->GetName().c_str(), isSelected))
					{
						std::string name = mat->GetName() + "_" + m_Context->SelectedEntity.GetComponent<TagComponent>().Tag;
						component.MaterialHandle = mat->GetUUID();
						component.GetInstanced(name);
					}

					// Set the initial focus when opening the combo
					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
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
					// TODO: Launch VS Code / Text Editor via OS system call
				}
			}

			ImGui::Separator();

			if (ImGui::BeginTable("MaterialProps", 2, ImGuiTableFlags_SizingFixedSame))
			{
				ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

				auto& shaderProps = material->GetShader()->GetProperties();
				for (auto& prop : shaderProps)
				{
					switch (prop.Type)
					{
					case ShaderPropertyType::Float:
					{
						RenderProperty<float>(prop, material, [&prop](const char* name, float* value) {
							return ImGui::DragFloat(name, value, prop.Step, prop.Min, prop.Max);
							});
						break;
					}
					case ShaderPropertyType::Float2:
					{
						RenderProperty<Vector2f>(prop, material, [&prop](const char* name, Vector2f* value) {
							return ImGui::DragFloat2(name, &value->x, prop.Step, prop.Min, prop.Max);
							});
						break;
					}
					case ShaderPropertyType::Float3:
					{
						RenderProperty<Vector3f>(prop, material, [&prop](const char* name, Vector3f* value) {
							return ImGui::DragFloat3(name, &value->x, prop.Step, prop.Min, prop.Max);
							});
						break;
					}
					case ShaderPropertyType::Float4:
					{
						RenderProperty<Vector4f>(prop, material, [&prop](const char* name, Vector4f* value) {
							return ImGui::DragFloat4(name, &value->x, prop.Step, prop.Min, prop.Max);
							});
						break;
					}
					case ShaderPropertyType::Color3:
					{
						RenderProperty<Vector3f>(prop, material, [](const char* name, Vector3f* value) {
							return ImGui::ColorEdit3(name, &value->x);
							});
						break;
					}
					case ShaderPropertyType::Color4:
					{
						RenderProperty<Vector4f>(prop, material, [](const char* name, Vector4f* value) {
							return ImGui::ColorEdit4(name, &value->x);
							});
						break;
					}
					case ShaderPropertyType::Slider:
					{
						RenderProperty<float>(prop, material, [&prop](const char* name, float* value) {
							return ImGui::SliderFloat(name, value, prop.Min, prop.Max);
							});
						break;
					}
					case ShaderPropertyType::Texture:
					{
						// TODO: Clean this up to look more like unities (need a clear button as well)
						if (ImGui::BeginTable("TextureDropTable", 2, ImGuiTableFlags_SizingFixedSame))
						{
							ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
							ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
							ImGui::TableNextRow();
							ImGui::TableNextColumn();

							// Big square are for texture preview
							auto emptyImage = Application::Instance().GetAssetManager().Load<Texture>("Ember-Forge/assets/icons/Empty.png");
							auto id = (void*)(intptr_t)(material->ContainsUniform(prop.UniformName)
								? std::get<SharedPtr<Texture>>(material->GetUniforms().at(prop.UniformName))->GetID()
								: emptyImage->GetID());
							ImGui::Image(id, ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0));
							if (ImGui::BeginDragDropTarget())
							{
								std::string payloadType = DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::AssetTexture);
								if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payloadType.c_str()))
								{
									EB_CORE_TRACE("Received payload with data: {}", (const char*)payload->Data);
									auto filePath = (const char*)payload->Data;
									auto texture = Application::Instance().GetAssetManager().Load<Texture>(filePath);
									material->SetUniform(prop.UniformName, texture);
								}
								ImGui::EndDragDropTarget();
							}

							//ImGui::PopID();
							ImGui::TableNextColumn();
							ImGui::AlignTextToFramePadding();
							ImGui::Text("Test text for now"); // TODO: Show texture name or "None" if no texture is set

							ImGui::EndTable();
						}
					}
					}
				}

				ImGui::EndTable();
			}
		}

	private:
		template<typename T, typename RenderFunc>
		void RenderProperty(const ShaderProperty& prop, const SharedPtr<MaterialBase>& material, RenderFunc renderFunc)
		{
			if (!material->ContainsUniform(prop.UniformName))
				return;

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text(prop.DisplayName.c_str());
			ImGui::TableNextColumn();
			ImGui::PushItemWidth(-FLT_MIN);

			std::string uniformLabel = ("##" + prop.UniformName);
			T value = std::get<T>(material->GetUniforms().at(prop.UniformName));
			if (renderFunc(uniformLabel.c_str(), &value))
			{
				if (prop.Normalize)
					value = Math::Normalize<T>(value, prop.Min, prop.Max);

				material->SetUniform(prop.UniformName, value);
			}
		}
	};

}