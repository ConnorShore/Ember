#include "ebpch.h"
#include "Layer.h"
#include "Application.h"

namespace Ember {

	void Layer::RegisterShader(const std::string& filePath)
	{
		Application::Instance().RegisterShader(filePath);
	}

	SharedPtr<Shader> Layer::GetShader(const std::string& name)
	{
		return Application::Instance().GetShader(name);
	}

	void Layer::RegisterTexture(const std::string& filePath)
	{
		Application::Instance().RegisterTexture(filePath);
	}

	SharedPtr<Texture> Layer::GetTexture(const std::string& name)
	{
		return Application::Instance().GetTexture(name);
	}

	void Layer::RegisterMesh(const std::string& filePath)
	{
		Application::Instance().RegisterMesh(filePath);
	}

	Ember::SharedPtr<Ember::Mesh> Layer::GetMesh(const std::string& name)
	{
		return Application::Instance().GetMesh(name);
	}

}
