#include "ebpch.h"
#include "ProjectManager.h"
#include "ProjectSerializer.h"
#include "Ember/Scene/SceneSerializer.h"

namespace Ember {

	SharedPtr<Project> ProjectManager::s_ActiveProject = nullptr;

	SharedPtr<Project> ProjectManager::NewProject(const std::string& filepath)
	{
		auto project = SharedPtr<Project>::Create(filepath);
		s_ActiveProject = project;

		ProjectSerializer serializer(s_ActiveProject);
		serializer.Serialize(filepath);

		// Create Assets folder and sub-asset folders
		std::filesystem::create_directories(project->GetAssetDirectory());
		std::filesystem::create_directories(project->GetAssetDirectory() / "Models");
		std::filesystem::create_directories(project->GetAssetDirectory() / "Materials");
		std::filesystem::create_directories(project->GetAssetDirectory() / "Shaders");
		std::filesystem::create_directories(project->GetAssetDirectory() / "Textures");
		std::filesystem::create_directories(project->GetAssetDirectory() / "Scripts");


		// TODO: Probably should copy default engine assets into this directory
		//  - Then update AssetManagerPanel to the new project 's asset directory (In editor layer)

		std::filesystem::create_directories(project->GetProjectDirectory() / "Scenes");

		// Create scenes directory and add default scene'
		std::string sceneName = s_ActiveProject->GetStartScenePath().filename().string();
		SceneSerializer sceneSerializer(SharedPtr<Scene>::Create(sceneName));

		sceneSerializer.Serialize(s_ActiveProject->GetStartScenePath().string());

		return s_ActiveProject;
	}

	SharedPtr<Project> ProjectManager::LoadProject(const std::string& filepath)
	{
		if (!std::filesystem::exists(filepath))
		{
			EB_CORE_ERROR("Project file does not exist: {}", filepath);
			return nullptr;
		}

		if (!s_ActiveProject)
		{
			s_ActiveProject = SharedPtr<Project>::Create(filepath);
		}

		ProjectSerializer serializer(s_ActiveProject);
		if (serializer.Deserialize(filepath))
		{
			// Need to reset asset managers state and load in the assets from this project
			return s_ActiveProject;
		}

		EB_CORE_ASSERT(false, "Failed to load project: {}", filepath);
		return nullptr;
	}

	void ProjectManager::SaveActiveProject()
	{
		if (!s_ActiveProject)
			return;

		std::string filename = s_ActiveProject->GetConfig().ProjectName + ".ebproj";
		std::filesystem::path filepath = s_ActiveProject->GetProjectDirectory() / filename;

		ProjectSerializer serializer(s_ActiveProject);
		serializer.Serialize(filepath.string());
	}

}