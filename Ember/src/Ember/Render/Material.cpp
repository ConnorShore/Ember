#include "ebpch.h"
#include "Material.h"
#include "Renderer3D.h"

namespace Ember {

	Material::Material(const std::string& name)
		: Material(name, Renderer3D::GetStandardGeometryShader(), RenderQueue::Opaque) 
	{
	}

	Material::Material(const std::string& name, const SharedPtr<Shader>& shader, const RenderQueue renderQueue)
		: Material(name, shader, renderQueue, {})
	{
	}

	Material::Material(const std::string& name, std::initializer_list<MaterialUniform> uniforms)
		: Material(name, Renderer3D::GetStandardGeometryShader(), RenderQueue::Opaque, uniforms)
	{
	}

	const SharedPtr<Material>& MaterialLibrary::RegisterMaterial(const std::string& name, const SharedPtr<Shader>& shader, const RenderQueue renderQueue)
	{
		auto material = SharedPtr<Material>::Create(name, shader, renderQueue);
		Add(name, std::move(material));
		return reinterpret_cast<const SharedPtr<Material>&>(Get(name));
	}

	const SharedPtr<Material>& MaterialLibrary::RegisterMaterial(const std::string& name, const SharedPtr<Shader>& shader, 
		const RenderQueue renderQueue, std::initializer_list<MaterialUniform> uniforms)
	{
		auto material = SharedPtr<Material>::Create(name, shader, renderQueue, uniforms);
		Add(name, std::move(material));
		return reinterpret_cast<const SharedPtr<Material>&>(Get(name));
	}

	const Ember::SharedPtr<Ember::Material>& MaterialLibrary::RegisterMaterial(const std::string& name)
	{
		auto material = SharedPtr<Material>::Create(name);
		Add(name, std::move(material));
		return reinterpret_cast<const SharedPtr<Material>&>(Get(name));
	}

	const Ember::SharedPtr<Ember::Material>& MaterialLibrary::RegisterMaterial(const std::string& name, std::initializer_list<MaterialUniform> uniforms)
	{
		auto material = SharedPtr<Material>::Create(name, uniforms);
		Add(name, std::move(material));
		return reinterpret_cast<const SharedPtr<Material>&>(Get(name));
	}

	const SharedPtr<MaterialInstance>& MaterialLibrary::RegisterInstance(const std::string& name, const SharedPtr<Material>& material)
	{
		auto instance = SharedPtr<MaterialInstance>::Create(name, material);
		Add(name, std::move(instance));
		return reinterpret_cast<const SharedPtr<MaterialInstance>&>(Get(name));
	}

	const SharedPtr<MaterialBase>& MaterialLibrary::Get(const std::string& name)
	{
		return m_MaterialMap[name];
	}

	bool MaterialLibrary::Exists(const std::string& name)
	{
		return m_MaterialMap.contains(name);
	}

	void MaterialLibrary::Add(const std::string& name, SharedPtr<MaterialBase>&& material)
	{
		EB_CORE_ASSERT(!Exists(name), "Material already exists in library!");
		m_MaterialMap[name] = std::move(material);
	}

}