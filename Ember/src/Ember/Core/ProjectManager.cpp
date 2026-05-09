#include "ebpch.h"
#include "ProjectManager.h"
#include "ProjectSerializer.h"
#include "Ember/Scene/SceneSerializer.h"
#include "Ember/Asset/AssetRegistrySerializer.h"

namespace Ember {

	SharedPtr<Project> ProjectManager::s_ActiveProject = nullptr;

	// Creates a new project on disk with the standard directory layout and a default scene
	SharedPtr<Project> ProjectManager::NewProject(const std::string& filepath)
	{
		auto project = SharedPtr<Project>::Create(filepath);
		s_ActiveProject = project;

		// Create collision filter manager
		auto& collisionFilterManager = s_ActiveProject->GetCollisionFilterManager();
		collisionFilterManager.InitWithFilters({
			"Environment",
			"VFX",
			"Player",
			"Enemy"
		});

		// Create render layer manager
		auto& renderLayerManager = s_ActiveProject->GetRenderFilterManager();
		renderLayerManager.InitWithFilters({
			"Default"
		});

		ProjectSerializer serializer(s_ActiveProject);
		serializer.Serialize(filepath);

		// Create Assets folder and sub-asset folders
		std::filesystem::create_directories(project->GetAssetDirectory());
		std::filesystem::create_directories(project->GetAssetDirectory() / "Models");
		std::filesystem::create_directories(project->GetAssetDirectory() / "Materials");
		std::filesystem::create_directories(project->GetAssetDirectory() / "Physics Materials");
		std::filesystem::create_directories(project->GetAssetDirectory() / "Shaders");
		std::filesystem::create_directories(project->GetAssetDirectory() / "Textures");
		std::filesystem::create_directories(project->GetAssetDirectory() / "Scripts");
		std::filesystem::create_directories(project->GetAssetDirectory() / "Prefabs");
		std::filesystem::create_directories(project->GetAssetDirectory() / "Fonts");
		std::filesystem::create_directories(project->GetAssetDirectory() / "Audio");


		// TODO: Probably should copy default engine assets into this directory
		//  - Then update AssetManagerPanel to the new project 's asset directory (In editor layer)

		std::filesystem::create_directories(project->GetScenesDirectory());

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
			EB_CORE_INFO("Created new active project instance for file: {}", filepath);
		}

		ProjectSerializer serializer(s_ActiveProject);
		if (!serializer.Deserialize(filepath))
		{
			EB_CORE_ASSERT(false, "Failed to load project: {}", filepath);
			return nullptr;
		}

		// Need to reset asset managers state and load in the assets from this project
		EB_CORE_INFO("Project deserialized successfully: {}", filepath);

		// Load project assets into the asset manager
		EB_CORE_INFO("Loading project assets for project: {}", filepath);
		EB_CORE_INFO("Active Project project directory: {}", s_ActiveProject->GetProjectDirectory().string());
		auto& assetManager = Application::Instance().GetAssetManager();
		assetManager.ClearAssets();
		assetManager.SetProjectAssetDirectory(s_ActiveProject->GetProjectDirectory());
		EB_CORE_INFO("Set Asset Manager project asset directory to: {}", assetManager.GetProjectAssetDirectory().string());
		EB_CORE_INFO("Asset files will be loaded from: {}", s_ActiveProject->GetAssetsFilePath().string());
		//assetManager.LoadDefaults();

		// Deserialize project assets
		EB_CORE_INFO("Deserializing project assets from path: {}", s_ActiveProject->GetAssetsFilePath().string());
		AssetRegistrySerializer assetSerializer(&assetManager);
		assetSerializer.Deserialize(s_ActiveProject->GetAssetsFilePath().string());

		return s_ActiveProject;
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

	void ProjectManager::ExportActiveProject(const std::filesystem::path& exportPath)
	{
		if (!s_ActiveProject)
		{
			EB_CORE_ERROR("No active project to export!");
			return;
		}

		EB_CORE_INFO("Exporting project to: {0}", exportPath.string());

		// 1. Create the base directory
		auto finalExportPath = exportPath / s_ActiveProject->GetConfig().ProjectName;
		std::filesystem::create_directories(finalExportPath);


		// Define our source paths (Assumes Editor working directory is the repo root)
		// TODO: Account for the different build types (i.e. release vs debug) and platforms (windows vs linux)
		std::filesystem::path runtimeExeSrc = "bin/Debug-windows-x86_64/Ember-Runtime/Ember-Runtime.exe";
		std::filesystem::path engineAssetSrc = Application::Instance().GetAssetManager().GetEngineAssetDirectory();
		std::filesystem::path projectAssetSrc = s_ActiveProject->GetAssetDirectory();
		std::filesystem::path projectFileSrc = s_ActiveProject->GetProjectFilePath();
		std::filesystem::path scenesSrc = s_ActiveProject->GetScenesDirectory();

		// 2. Define the destination paths
		// We rename the runtime exe to match the project name!
		std::filesystem::path exeDest = finalExportPath / (s_ActiveProject->GetConfig().ProjectName + ".exe");
		std::filesystem::path engineAssetDest = finalExportPath / "EmberCore";
		std::filesystem::path projectAssetDest = finalExportPath / "GameData/Assets";

		std::filesystem::path scenesDest = finalExportPath / "GameData/Scenes";
		std::filesystem::path projectFileDest = finalExportPath / "project.ebproj";

		try
		{
			auto copyOptions = std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive;

			// Copy the Executable
			std::filesystem::copy_file(runtimeExeSrc, exeDest, std::filesystem::copy_options::overwrite_existing);

			// Copy the Project File
			std::filesystem::copy_file(projectFileSrc, projectFileDest, std::filesystem::copy_options::overwrite_existing);

			// Copy the Scenes Folder
			std::filesystem::create_directories(scenesDest);
			std::filesystem::copy(scenesSrc, scenesDest, copyOptions);

			// 5. Copy the Asset Folders
			std::filesystem::create_directories(engineAssetDest);
			std::filesystem::copy(engineAssetSrc, engineAssetDest, copyOptions);

			std::filesystem::create_directories(projectAssetDest);
			std::filesystem::copy(projectAssetSrc, projectAssetDest, copyOptions);

			EB_CORE_INFO("Export Complete!");
		}
		catch (const std::filesystem::filesystem_error& e)
		{
			EB_CORE_ERROR("Export failed: {0}", e.what());
		}
	}

}