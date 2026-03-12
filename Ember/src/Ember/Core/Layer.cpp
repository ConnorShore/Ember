#include "ebpch.h"
#include "Layer.h"
#include "Application.h"

namespace Ember {

	const SharedPtr<Shader>& Layer::RegisterShader(const std::string& filePath)
	{
		return Application::Instance().RegisterShader(filePath);
	}

	const SharedPtr<Shader>& Layer::GetShader(const std::string& name)
	{
		return Application::Instance().GetShader(name);
	}

	const SharedPtr<Texture>& Layer::RegisterTexture(const std::string& filePath)
	{
		return Application::Instance().RegisterTexture(filePath);
	}

	const SharedPtr<Texture>& Layer::GetTexture(const std::string& name)
	{
		return Application::Instance().GetTexture(name);
	}

	const SharedPtr<Mesh>& Layer::RegisterMesh(const std::string& filePath)
	{
		return Application::Instance().RegisterMesh(filePath);
	}

	const SharedPtr<Mesh>& Layer::GetMesh(const std::string& name)
	{
		return Application::Instance().GetMesh(name);
	}

	const SharedPtr<Material>& Layer::RegisterMaterial(const std::string& name, SharedPtr<Shader> shader)
	{
		return Application::Instance().RegisterMaterial(name, shader);
	}

	const Ember::SharedPtr<Ember::Material>& Layer::RegisterMaterial(const std::string& name, SharedPtr<Shader> shader, std::initializer_list<MaterialUniform> uniforms)
	{
		return Application::Instance().RegisterMaterial(name, shader, uniforms);
	}

	const SharedPtr<MaterialInstance>& Layer::RegisterMaterial(const std::string& name, SharedPtr<Material> material)
	{
		return Application::Instance().RegisterMaterial(name, material);
	}

	const SharedPtr<MaterialBase>& Layer::GetMaterial(const std::string& name)
	{
		return Application::Instance().GetMaterial(name);
	}

}
