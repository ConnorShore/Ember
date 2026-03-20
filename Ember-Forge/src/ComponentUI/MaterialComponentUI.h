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
			auto defaultMaterial = m_Context->ActiveScene->GetAsset<Material>(Constants::Assets::StandardGeometryMat);
			MaterialComponent comp{ defaultMaterial };
			m_Context->ActiveScene->AttachComponent(entity, comp);
		}


	protected:
		inline void RenderComponentImpl(MaterialComponent& component) override
		{
			auto& material = component.Material;
			if (!material)
				return;

			ImGui::Text("Material: %s", material->GetName().c_str());
			ImGui::Text("Shader: %s", material->GetShader()->GetName().c_str());
			ImGui::Separator();
			ImGui::Text("Shader Properties:");
			ImGui::Separator();

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
				}
			}
		}

	private:
		template<typename T, typename RenderFunc>
		void RenderProperty(const ShaderProperty& prop, const SharedPtr<MaterialBase>& material, RenderFunc renderFunc)
		{
			if (!material->ContainsUniform(prop.UniformName))
				return;

			T value = std::get<T>(material->GetUniforms().at(prop.UniformName));
			if (renderFunc(prop.DisplayName.c_str(), &value))
			{
				if (prop.Normalize)
					value = Math::Normalize<T>(value, prop.Min, prop.Max);

				material->SetUniform(prop.UniformName, value);
			}
		}
	};

}