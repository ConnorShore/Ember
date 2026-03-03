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

}
