#include "ebpch.h"
#include "Material.h"
#include "Renderer3D.h"

namespace Ember {

	Material::Material(UUID uuid, const std::string& name, const SharedPtr<Shader>& shader, const RenderQueue renderQueue)
		: Material(uuid, name, shader, renderQueue, {})
	{
	}

	Material::Material(const std::string& name, const SharedPtr<Shader>& shader, const RenderQueue renderQueue)
		: Material(UUID(), name, shader, renderQueue, {})
	{
	}


	Material::Material(const std::string& name)
		: Material(name, nullptr, RenderQueue::None, {})
	{
	}

	const SharedPtr<Material>& MaterialLibrary::RegisterMaterial(UUID uuid, const std::string& name, const SharedPtr<Shader>& shader, const RenderQueue renderQueue)
	{
		auto material = SharedPtr<Material>::Create(uuid, name, shader, renderQueue);
		Add(name, std::move(material));
		return DynamicPointerCast<Material>(Get(name));
	}

	const SharedPtr<Material>& MaterialLibrary::RegisterMaterial(const std::string& name, const SharedPtr<Shader>& shader, const RenderQueue renderQueue)
	{
		return RegisterMaterial(UUID(), name, shader, renderQueue);
	}

	const SharedPtr<Material>& MaterialLibrary::RegisterMaterial(const std::string& name, const SharedPtr<Shader>& shader, 
		const RenderQueue renderQueue, std::initializer_list<MaterialUniform> uniforms)
	{
		auto material = SharedPtr<Material>::Create(name, shader, renderQueue, uniforms);
		Add(name, std::move(material));
		return DynamicPointerCast<Material>(Get(name));
	}

	const Ember::SharedPtr<Ember::Material>& MaterialLibrary::RegisterMaterial(const std::string& name)
	{
		auto material = SharedPtr<Material>::Create(name);
		Add(name, std::move(material));
		return DynamicPointerCast<Material>(Get(name));
	}

	const Ember::SharedPtr<Ember::Material>& MaterialLibrary::RegisterMaterial(const std::string& name, std::initializer_list<MaterialUniform> uniforms)
	{
		auto material = SharedPtr<Material>::Create(name, uniforms);
		Add(name, std::move(material));
		return DynamicPointerCast<Material>(Get(name));
	}

	const SharedPtr<MaterialInstance>& MaterialLibrary::RegisterInstance(const std::string& name, const SharedPtr<Material>& material)
	{
		auto instance = SharedPtr<MaterialInstance>::Create(name, material);
		Add(name, std::move(instance));
		return DynamicPointerCast<MaterialInstance>(Get(name));
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