#pragma once

#include "Ember/Core/Core.h"
#include "Project.h"

#include <string>

namespace Ember {

	class ProjectSerializer
	{
	public:
		ProjectSerializer(const SharedPtr<Project>& project) : m_Project(project) {}
		~ProjectSerializer() = default;

		bool Serialize(const std::string& filePath);
		bool Deserialize(const std::string& filePath);

	private:
		SharedPtr<Project> m_Project;
	};

}