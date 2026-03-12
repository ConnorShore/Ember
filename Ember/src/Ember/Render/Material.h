#pragma once

#include "Ember/Core/Core.h"
#include "Ember/Math/Math.h"
#include "Shader.h"
#include "Texture.h"

#include <unordered_map>
#include <variant>
#include <string>
#include <tuple>

namespace Ember {

	//////////////////////////////////////////////////////////////////////////
	// Material Base
	//////////////////////////////////////////////////////////////////////////

	class MaterialBase : public SharedResource
	{
	public:
		virtual ~MaterialBase() = default;
		virtual void Bind() const = 0;

		virtual const SharedPtr<Shader> GetShader() const = 0;
		virtual const std::string& GetName() const = 0;
	};

	//////////////////////////////////////////////////////////////////////////
	// Material
	//////////////////////////////////////////////////////////////////////////

	using MaterialValue = std::variant<
		int,
		float,
		Vector2f,
		Vector3f,
		Vector4f,
		Matrix4f,
		SharedPtr<Texture>
	>;

	using MaterialUniform = std::tuple<std::string, MaterialValue>;

	class Material : public MaterialBase
	{
	public:
		Material(const std::string& name, const SharedPtr<Shader>& shader) : m_Name(name), m_Shader(shader) {}
		Material(const std::string& name, const SharedPtr<Shader>& shader, std::initializer_list<MaterialUniform> uniforms)
			: m_Name(name), m_Shader(shader) 
		{
			for (auto u : uniforms)
			{
				Set(std::get<0>(u), std::get<1>(u));
			}
		}
		virtual ~Material() = default;

		inline void Bind() const
		{
			m_Shader->Bind();

			unsigned int textureSlot = 0;
			for (auto [name, value] : m_Uniforms)
			{
				UploadUniform(name, value, textureSlot);
			}
		}

		template<typename T>
		void Set(const std::string& name, const T& value) { m_Uniforms[name] = value; }

		inline void UploadUniform(const std::string& name, const MaterialValue& value, unsigned int& textureSlot) const
		{
			if (std::holds_alternative<int>(value)) m_Shader->SetInt(name, std::get<int>(value));
			else if (std::holds_alternative<float>(value)) m_Shader->SetFloat(name, std::get<float>(value));
			else if (std::holds_alternative<Vector2f>(value)) m_Shader->SetFloat2(name, std::get<Vector2f>(value));
			else if (std::holds_alternative<Vector3f>(value)) m_Shader->SetFloat3(name, std::get<Vector3f>(value));
			else if (std::holds_alternative<Vector4f>(value)) m_Shader->SetFloat4(name, std::get<Vector4f>(value));
			else if (std::holds_alternative<SharedPtr<Texture>>(value))
			{
				auto& tex = std::get<SharedPtr<Texture>>(value);
				tex->Bind();
				m_Shader->SetInt(name, textureSlot++);
			}
			else EB_CORE_ASSERT(false, "Unknown Material Value type!");
		}

		inline const SharedPtr<Shader> GetShader() const { return m_Shader; }

		inline const std::string& GetName() const override { return m_Name; }

	private:
		std::string m_Name;
		SharedPtr<Shader> m_Shader;
		std::unordered_map<std::string, MaterialValue> m_Uniforms;
	};

	//////////////////////////////////////////////////////////////////////////
	// Material Instance
	//////////////////////////////////////////////////////////////////////////

	class MaterialInstance : public MaterialBase
	{
	public:
		MaterialInstance(const std::string& name, const SharedPtr<Material>& material) : m_Name(name), m_Material(material) {}
		MaterialInstance(const std::string& name, const SharedPtr<Material>& material, std::initializer_list<MaterialUniform> uniforms)
			: m_Name(name), m_Material(material)
		{
			for (auto u : uniforms)
			{
				Set(std::get<0>(u), std::get<1>(u));
			}
		}
		virtual ~MaterialInstance() = default;

		template<typename T>
		void Set(const std::string& name, const T& value) { m_Uniforms[name] = value; }

		void Bind() const
		{
			m_Material->GetShader()->Bind();

			unsigned int textureSlot = 0;
			for (auto [name, value] : m_Uniforms)
			{
				m_Material->UploadUniform(name, value, textureSlot);
			}
		}

		inline const std::string& GetName() const override { return m_Name; }

		inline const SharedPtr<Shader> GetShader() const { return m_Material->GetShader(); }

	private:
		std::string m_Name;
		SharedPtr<Material> m_Material;
		std::unordered_map<std::string, MaterialValue> m_Uniforms;
	};

	//////////////////////////////////////////////////////////////////////////
	// Material Library
	//////////////////////////////////////////////////////////////////////////

	class MaterialLibrary
	{
	public:
		const SharedPtr<Material>& RegisterMaterial(const std::string& name, const SharedPtr<Shader>& shader);
		const SharedPtr<Material>& RegisterMaterial(const std::string& name, const SharedPtr<Shader>& shader, std::initializer_list<MaterialUniform> uniforms);
		const SharedPtr<MaterialInstance>& RegisterInstance(const std::string& name, const SharedPtr<Material>& material);

		const SharedPtr<MaterialBase>& Get(const std::string& name);
		bool Exists(const std::string& name);

	private:
		void Add(const std::string& name, SharedPtr<MaterialBase>&& material);
	private:
		std::unordered_map<std::string, SharedPtr<MaterialBase>> m_MaterialMap;
	};

}