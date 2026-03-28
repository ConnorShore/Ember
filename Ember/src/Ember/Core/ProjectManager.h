#pragma once

#include "Project.h"
#include "Ember/Core/Core.h"

namespace Ember {

	class ProjectManager
	{
	public:
		static SharedPtr<Project> NewProject(const std::string& filepath);
		static SharedPtr<Project> LoadProject(const std::string& filepath);
		static void SaveActiveProject();

		static SharedPtr<Project> GetActive() { return s_ActiveProject; }

	private:
		static SharedPtr<Project> s_ActiveProject;
	};

}