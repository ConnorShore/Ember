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
			SharedPtr<Material> baseMaterial = nullptr;
			SharedPtr<MaterialInstance> instancedMaterial = nullptr;

			if (auto inst = DynamicPointerCast<MaterialInstance>(component.Material))
			{
				instancedMaterial = inst;
				baseMaterial = inst->GetMaterial();
			}
			else if (auto mat = DynamicPointerCast<Material>(component.Material))
			{
				baseMaterial = mat;
			}
			else
			{
				EB_CORE_ASSERT(false, "Unknown Material type in MaterialComponentUI!");
				return;
			}

			ImGui::Text("Material: %s", component.Material->GetName().c_str());
			ImGui::Text("Shader: %s", baseMaterial->GetShader()->GetName().c_str());
			ImGui::Separator();
			ImGui::Text("Shader Properties:");
			ImGui::Separator();

			auto& shaderProps = baseMaterial->GetShader()->GetProperties();

			// 2. Point to the correct memory dictionary!
			auto& activeUniforms = instancedMaterial ? instancedMaterial->GetUniforms() : baseMaterial->GetUniforms();

			for (auto& prop : shaderProps)
			{
				if (activeUniforms.find(prop.UniformName) == activeUniforms.end())
					continue;

				switch (prop.Type)
				{
				case ShaderPropertyType::Float:
				{
					RenderProperty<float>(prop, activeUniforms, instancedMaterial, baseMaterial, [](const char* name, float* value) {
						return ImGui::DragFloat(name, value, 0.01f);
						});
					break;
				}
				case ShaderPropertyType::Float2:
				{
					RenderProperty<Vector2f>(prop, activeUniforms, instancedMaterial, baseMaterial, [](const char* name, Vector2f* value) {
						return ImGui::DragFloat2(name, &value->x, 0.01f);
						});
					break;
				}
				case ShaderPropertyType::Float3:
				{
					RenderProperty<Vector3f>(prop, activeUniforms, instancedMaterial, baseMaterial, [](const char* name, Vector3f* value) {
						return ImGui::DragFloat3(name, &value->x, 0.01f);
						});
					break;
				}
				case ShaderPropertyType::Float4:
				{
					RenderProperty<Vector4f>(prop, activeUniforms, instancedMaterial, baseMaterial, [](const char* name, Vector4f* value) {
						return ImGui::DragFloat4(name, &value->x, 0.01f);
						});
					break;
				}
				case ShaderPropertyType::Color3:
				{
					RenderProperty<Vector3f>(prop, activeUniforms, instancedMaterial, baseMaterial, [](const char* name, Vector3f* value) {
						return ImGui::ColorEdit3(name, &value->x);
						});
					break;
				}
				case ShaderPropertyType::Color4:
				{
					RenderProperty<Vector4f>(prop, activeUniforms, instancedMaterial, baseMaterial, [](const char* name, Vector4f* value) {
						return ImGui::ColorEdit4(name, &value->x);
						});
					break;
				}
				case ShaderPropertyType::Slider:
				{
					RenderProperty<float>(prop, activeUniforms, instancedMaterial, baseMaterial, [](const char* name, float* value) {
						return ImGui::SliderFloat(name, value, 0.0f, 1.0f);
						});
					break;
				}
				}
			}
		}

	private:
		template<typename T, typename RenderFunc>
		void RenderProperty(const ShaderProperty& prop,
			const std::unordered_map<std::string, MaterialValue>& activeUniforms,
			SharedPtr<MaterialInstance>& instancedMat,
			SharedPtr<Material>& baseMat,
			RenderFunc renderFunc)
		{
			T value = std::get<T>(activeUniforms.at(prop.UniformName));

			if (renderFunc(prop.DisplayName.c_str(), &value))
			{
				if (instancedMat) instancedMat->SetUniform(prop.UniformName, value);
				else baseMat->SetUniform(prop.UniformName, value);
			}
		}
	};

}