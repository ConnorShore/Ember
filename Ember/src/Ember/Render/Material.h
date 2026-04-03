#pragma once

#include "Ember/Core/Core.h"
#include "Ember/Math/Math.h"
#include "Ember/Asset/Asset.h"
#include "Shader.h"
#include "Texture.h"

#include <unordered_map>
#include <variant>
#include <string>
#include <tuple>

namespace Ember {

	//////////////////////////////////////////////////////////////////////////
	// Render Queue Type
	//////////////////////////////////////////////////////////////////////////

	enum class RenderQueue : uint8_t
	{
		None = 0,
		Opaque,
		Forward,
		Transparent
	};

	//////////////////////////////////////////////////////////////////////////
	// Material Value
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

	//////////////////////////////////////////////////////////////////////////
	// Material Base
	//////////////////////////////////////////////////////////////////////////

	class MaterialBase : public Asset
	{
	public:
		MaterialBase(UUID uuid, const std::string& name, const SharedPtr<Shader>& shader, RenderQueue renderQueue)
			: Asset(uuid, name, "", GetStaticType()), m_Shader(shader), m_RenderQueue(renderQueue) {}
		MaterialBase(const std::string& name, const SharedPtr<Shader>& shader, RenderQueue renderQueue)
			: Asset(name, "", GetStaticType()), m_Shader(shader), m_RenderQueue(renderQueue) {}

		virtual ~MaterialBase() = default;
		virtual void Bind() const = 0;

		inline const RenderQueue GetRenderQueue() const { return m_RenderQueue; }
		inline void SetRenderQueue(const RenderQueue renderQueue) { m_RenderQueue = renderQueue; }

		inline const SharedPtr<Shader> GetShader() const { return m_Shader; }
		inline void SetShader(const SharedPtr<Shader>& shader) { m_Shader = shader; }

		virtual void SetUniform(const std::string& name, const MaterialValue& value) = 0;
		virtual const std::unordered_map<std::string, MaterialValue>& GetUniforms() const = 0;

		virtual bool ContainsUniform(const std::string& name) const = 0;

		static AssetType GetStaticType() { return AssetType::Material; }

	protected:
		SharedPtr<Shader> m_Shader;
		RenderQueue m_RenderQueue;
	};

	//////////////////////////////////////////////////////////////////////////
	// Material
	//////////////////////////////////////////////////////////////////////////

	using MaterialUniform = std::tuple<std::string, MaterialValue>;

	class Material : public MaterialBase
	{
	public:
		Material(UUID uuid, const std::string& name, const SharedPtr<Shader>& shader, const RenderQueue renderQueue, std::initializer_list<MaterialUniform> uniforms)
			: MaterialBase(uuid, name, shader, renderQueue)
		{
			for (auto u : uniforms)
			{
				SetUniform(std::get<0>(u), std::get<1>(u));
			}
		}
		Material(const std::string& name, const SharedPtr<Shader>& shader, const RenderQueue renderQueue, std::initializer_list<MaterialUniform> uniforms)
			: Material(UUID(), name, shader, renderQueue, uniforms)
		{
		}

		Material(UUID uuid, const std::string& name, std::initializer_list<MaterialUniform> uniforms)
			: Material(uuid, name, nullptr, RenderQueue::None, uniforms)
		{
		}
		Material(UUID uuid, const std::string& name, const SharedPtr<Shader>& shader, const RenderQueue renderQueue);

		Material(const std::string& name, std::initializer_list<MaterialUniform> uniforms)
			: Material(UUID(), name, nullptr, RenderQueue::None, uniforms)
		{
		}
		Material(const std::string& name, const SharedPtr<Shader>& shader, const RenderQueue renderQueue);
		Material(const std::string& name);

		virtual ~Material() = default;

		inline void Bind() const override
		{
			m_Shader->Bind();

			// Track texture slot so each texture uniform gets a unique binding point
			uint32_t textureSlot = 0;
			for (auto [name, value] : m_Uniforms)
			{
				UploadUniform(name, value, textureSlot);
			}
		}

		inline void SetUniform(const std::string& name, const MaterialValue& value) override { m_Uniforms[name] = value; }
		inline const std::unordered_map<std::string, MaterialValue>& GetUniforms() const override { return m_Uniforms; }
		inline bool ContainsUniform(const std::string& name) const override { return m_Uniforms.find(name) != m_Uniforms.end(); }

		// Uploads a single uniform to the GPU, dispatching by variant type
		inline void UploadUniform(const std::string& name, const MaterialValue& value, uint32_t& textureSlot) const
		{
			if (std::holds_alternative<int>(value)) m_Shader->SetInt(name, std::get<int>(value));
			else if (std::holds_alternative<float>(value)) m_Shader->SetFloat(name, std::get<float>(value));
			else if (std::holds_alternative<Vector2f>(value)) m_Shader->SetFloat2(name, std::get<Vector2f>(value));
			else if (std::holds_alternative<Vector3f>(value)) m_Shader->SetFloat3(name, std::get<Vector3f>(value));
			else if (std::holds_alternative<Vector4f>(value)) m_Shader->SetFloat4(name, std::get<Vector4f>(value));
			else if (std::holds_alternative<SharedPtr<Texture>>(value))
			{
				auto& tex = std::get<SharedPtr<Texture>>(value);
				tex->Bind(textureSlot);
				m_Shader->SetInt(name, textureSlot++);
			}
			else EB_CORE_ASSERT(false, "Unknown Material Value type!");
		}


	private:
		std::unordered_map<std::string, MaterialValue> m_Uniforms;
	};

	//////////////////////////////////////////////////////////////////////////
	// Material Instance
	//////////////////////////////////////////////////////////////////////////

	class MaterialInstance : public MaterialBase
	{
	public:
		MaterialInstance(const std::string& name, const SharedPtr<Material>& material, std::initializer_list<MaterialUniform> uniforms)
			: MaterialBase(name, material->GetShader(), material->GetRenderQueue()), m_Material(material)
		{
			for (auto u : uniforms)
			{
				SetUniform(std::get<0>(u), std::get<1>(u));
			}
		}
		MaterialInstance(UUID uuid, const std::string& name, const SharedPtr<Material>& material)
			: MaterialBase(uuid, name, material->GetShader(), material->GetRenderQueue()),
			m_Material(material),
			m_Uniforms(material->GetUniforms()) {
		}
		MaterialInstance(const std::string& name, const SharedPtr<Material>& material)
			: MaterialInstance(UUID(), name, material) {}

		virtual ~MaterialInstance() = default;

		void SetUniform(const std::string& name, const MaterialValue& value) override { m_Uniforms[name] = value; }
		bool ContainsUniform(const std::string& name) const override { return m_Uniforms.find(name) != m_Uniforms.end(); }
		const std::unordered_map<std::string, MaterialValue>& GetUniforms() const override { return m_Uniforms; }

		void Bind() const override
		{
			m_Material->GetShader()->Bind();

			uint32_t textureSlot = 0;
			for (auto [name, value] : m_Uniforms)
			{
				m_Material->UploadUniform(name, value, textureSlot);
			}
		}

		inline const SharedPtr<Material> GetMaterial() const { return m_Material; }

	private:
		SharedPtr<Material> m_Material;
		std::unordered_map<std::string, MaterialValue> m_Uniforms;
	};

	//////////////////////////////////////////////////////////////////////////
	// Material Library
	//////////////////////////////////////////////////////////////////////////

	class MaterialLibrary
	{
	public:
		SharedPtr<Material> RegisterMaterial(const std::string& name);
		SharedPtr<Material> RegisterMaterial(UUID uuid, const std::string& name, const SharedPtr<Shader>& shader, const RenderQueue renderQueue);
		SharedPtr<Material> RegisterMaterial(const std::string& name, const SharedPtr<Shader>& shader, const RenderQueue renderQueue);
		SharedPtr<Material> RegisterMaterial(const std::string& name, const SharedPtr<Shader>& shader, 
			const RenderQueue renderQueue, std::initializer_list<MaterialUniform> uniforms);
		SharedPtr<Material> RegisterMaterial(const std::string& name, std::initializer_list<MaterialUniform> uniforms);
		SharedPtr<MaterialInstance> RegisterInstance(const std::string& name, const SharedPtr<Material>& material);

		const SharedPtr<MaterialBase>& Get(const std::string& name);
		bool Exists(const std::string& name);

	private:
		void Add(const std::string& name, SharedPtr<MaterialBase>&& material);
	private:
		std::unordered_map<std::string, SharedPtr<MaterialBase>> m_MaterialMap;
	};

}