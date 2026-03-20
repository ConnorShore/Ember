#pragma once
#include "ComponentUI.h"
#include <variant>

namespace Ember {

	class MaterialComponentUI : public ComponentUI<MaterialComponent>
	{
	public:
		inline const char* GetName() const override { return "Material Component"; }

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
					RenderProperty<float>(prop, material, [](const char* name, float* value) {
						return ImGui::DragFloat(name, value, 0.01f);
						});
					break;
				}
				case ShaderPropertyType::Float2:
				{
					RenderProperty<Vector2f>(prop, material, [](const char* name, Vector2f* value) {
						return ImGui::DragFloat2(name, &value->x, 0.01f);
						});
					break;
				}
				case ShaderPropertyType::Float3:
				{
					RenderProperty<Vector3f>(prop, material, [](const char* name, Vector3f* value) {
						return ImGui::DragFloat3(name, &value->x, 0.01f);
						});
					break;
				}
				case ShaderPropertyType::Float4:
				{
					RenderProperty<Vector4f>(prop, material, [](const char* name, Vector4f* value) {
						return ImGui::DragFloat4(name, &value->x, 0.01f);
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
					RenderProperty<float>(prop, material, [](const char* name, float* value) {
						return ImGui::SliderFloat(name, value, 0.0f, 1.0f);
						});
					break;
				}
				}
			}
		}

	private:
		template<typename T, typename RenderFunc>
		void RenderProperty(const ShaderProperty& prop, const SharedPtr<MaterialBase>& material, float interval, float min, float max, bool normalize, RenderFunc renderFunc)
		{
			if (!material->ContainsUniform(prop.UniformName))
				return;

			T value = std::get<T>(material->GetUniforms().at(prop.UniformName));
			if (renderFunc(prop.DisplayName.c_str(), &value, interval, min, max))
			{
				if (normalize)
					value = Math::Normalize<T>(value, min, max);

				material->SetUniform(prop.UniformName, value);
			}
		}

		template<typename T, typename RenderFunc>
		void RenderProperty(const ShaderProperty& prop, const SharedPtr<MaterialBase>& material, RenderFunc renderFunc)
		{
			auto wrappedLambda = [renderFunc](const char* name, T* val, float i, float mn, float mx) {
				return renderFunc(name, val);
				};
			RenderProperty<T>(prop, material, 0.1f, 0.0f, 1.0f, false, wrappedLambda);
		}
	};

}