#pragma once

#include "Ember/Core/Core.h"
#include "Scene.h"

#include <string>

namespace Ember {

	class SceneSerializer
	{
	public:
		SceneSerializer(const SharedPtr<Scene>& scene) : m_Scene(scene) {}
		bool Serialize(const std::string& filepath);
		bool Deserialize(const std::string& filepath);

	private:
		SharedPtr<Scene> m_Scene;
	};

}