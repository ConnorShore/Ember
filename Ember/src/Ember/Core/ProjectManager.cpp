#include "ebpch.h"
#include "ProjectManager.h"
#include "ProjectSerializer.h"
#include "Ember/Asset/Serializers/AssetRegistrySerializer.h"
#include "Ember/Asset/Serializers/MaterialSerializer.h"
#include "Ember/Asset/Serializers/ModelSerializer.h"
#include "Ember/Asset/Serializers/MeshSerializer.h"
#include "Ember/Asset/Serializers/PhysicsMaterialSerializer.h"
#include "Ember/Asset/Serializers/NavigationMeshSerializer.h"
#include "Ember/Asset/Serializers/SkeletonSerializer.h"
#include "Ember/Asset/Serializers/SkeletonMaskSerializer.h"
#include "Ember/Asset/NavigationMeshData.h"
#include "Ember/Animation/AnimationSerializer.h"
#include "Ember/Animation/AnimationControllerSerializer.h"
#include "Ember/Render/TextureImporter.h"
#include "Ember/Render/Shader.h"
#include "Ember/Core/Application.h"
#include "Ember/Scene/Scene.h"
#include "Ember/Scene/SceneSerializer.h"

namespace Ember {
	namespace {
		template<typename T, typename Fn>
		void CookAssets(const AssetManager& assetManager, Fn&& cookFn)
		{
			for (const auto& asset : assetManager.GetAssetsOfType<T>())
			{
				if (!asset || asset->GetFilePath().empty())
					continue;
				cookFn(asset);
			}
		}

		void CookProjectAssets(AssetManager& assetManager)
		{
			CookAssets<Texture>(assetManager, [](const SharedPtr<Texture>& textureAsset) {
				auto texture2D = DynamicPointerCast<Texture2D>(textureAsset);
				if (texture2D)
					TextureImporter::SaveCooked(texture2D, texture2D->GetFilePath());
			});

			CookAssets<Shader>(assetManager, [](const SharedPtr<Shader>& shaderAsset) {
				ShaderImporter::SaveCooked(shaderAsset, shaderAsset->GetFilePath());
			});

			CookAssets<Mesh>(assetManager, [](const SharedPtr<Mesh>& meshAsset) {
				MeshSerializer::SerializeCooked(meshAsset->GetFilePath(), meshAsset);
			});

			CookAssets<Model>(assetManager, [](const SharedPtr<Model>& modelAsset) {
				ModelSerializer::SerializeCooked(modelAsset->GetFilePath(), modelAsset);
			});

			CookAssets<Skeleton>(assetManager, [](const SharedPtr<Skeleton>& skeletonAsset) {
				SkeletonSerializer::SerializeCooked(skeletonAsset->GetFilePath(), skeletonAsset);
			});

			CookAssets<SkeletonMask>(assetManager, [](const SharedPtr<SkeletonMask>& maskAsset) {
				SkeletonMaskSerializer::SerializeCooked(maskAsset->GetFilePath(), maskAsset);
			});

			CookAssets<Animation>(assetManager, [](const SharedPtr<Animation>& animationAsset) {
				AnimationSerializer::SerializeCooked(animationAsset->GetFilePath(), animationAsset);
			});

			CookAssets<AnimationController>(assetManager, [](const SharedPtr<AnimationController>& controllerAsset) {
				AnimationControllerSerializer::SerializeCooked(controllerAsset->GetFilePath(), controllerAsset);
			});

			CookAssets<PhysicsMaterial>(assetManager, [](const SharedPtr<PhysicsMaterial>& materialAsset) {
				PhysicsMaterialSerializer::SerializeCooked(materialAsset->GetFilePath(), materialAsset);
			});

			CookAssets<NavigationMeshData>(assetManager, [](const SharedPtr<NavigationMeshData>& navMeshAsset) {
				NavigationMeshSerializer::SerializeCooked(navMeshAsset->GetFilePath(), navMeshAsset);
			});

			CookAssets<Material>(assetManager, [&](const SharedPtr<Material>& materialAsset) {
				MaterialSerializer::SerializeCooked(materialAsset->GetFilePath(), materialAsset);
			});

			CookAssets<Scene>(assetManager, [](const SharedPtr<Scene>& sceneAsset) {
				SceneSerializer(sceneAsset).SerializeCooked(sceneAsset->GetFilePath());
				});
		}
	}

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

		// Clear existing assets from the asset manager and point it to the new project's asset directory
		Application::Instance().GetAssetManager().ClearAssets();

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
		std::filesystem::create_directories(project->GetAssetDirectory() / "NavMeshes");

		std::filesystem::create_directories(project->GetScenesDirectory());

		// Create the default scene and register it in the asset manager so it gets serialized into assets.eba
		std::string defaultSceneName = project->GetStartScenePath().stem().string();
		Application::Instance().GetSceneManager().CreateScene(defaultSceneName);

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
		auto& assetManager = Application::Instance().GetAssetManager();
		assetManager.ClearAssets();
		assetManager.SetProjectAssetDirectory(s_ActiveProject->GetProjectDirectory());

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

	bool ProjectManager::ExportActiveProject(const std::filesystem::path& exportPath)
	{
		if (!s_ActiveProject)
		{
			EB_CORE_ERROR("No active project to export!");
			return false;
		}

		EB_CORE_INFO("Exporting project to: {0}", exportPath.string());

		// Create the base directory
		auto finalExportPath = exportPath / s_ActiveProject->GetConfig().ProjectName;
		std::filesystem::create_directories(finalExportPath);

		// TODO: account for different architectures and systems
		std::string configFolder;
#if defined(EB_DEBUG)
		configFolder = "Debug-windows-x86_64";
#elif defined(EB_RELEASE)
		configFolder = "Release-windows-x86_64";
#elif defined(EB_DIST)
		configFolder = "Dist-windows-x86_64";
#else
		configFolder = "Debug-windows-x86_64";
#endif

		// Define our source paths (Assumes Editor working directory is the repo root)
		std::filesystem::path runtimeExeSrc = "bin/" + configFolder + "/Ember-Runtime/Ember-Runtime.exe";
		std::filesystem::path engineAssetSrc = Application::Instance().GetAssetManager().GetEngineAssetDirectory();
		std::filesystem::path projectAssetSrc = s_ActiveProject->GetAssetDirectory();
		std::filesystem::path projectFileSrc = s_ActiveProject->GetProjectFilePath();
		std::filesystem::path scenesSrc = s_ActiveProject->GetScenesDirectory();

		// Define the destination paths
		// We rename the runtime exe to match the project name!
		std::filesystem::path exeDest = finalExportPath / (s_ActiveProject->GetConfig().ProjectName + ".exe");
		std::filesystem::path engineAssetDest = finalExportPath / "EmberCore";
		std::filesystem::path projectAssetDest = finalExportPath / "GameData/Assets";

		std::filesystem::path scenesDest = finalExportPath / "GameData/Scenes";
		std::filesystem::path projectFileDest = finalExportPath / "project.ebproj";

		try
		{
			auto copyOptions = std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive;

			// Generate cooked sidecar binaries before copying the asset trees.
			CookProjectAssets(Application::Instance().GetAssetManager());

			// Copy the Executable
			std::filesystem::copy_file(runtimeExeSrc, exeDest, std::filesystem::copy_options::overwrite_existing);

			// Copy the Project File
			std::filesystem::copy_file(projectFileSrc, projectFileDest, std::filesystem::copy_options::overwrite_existing);

			// Copy the Scenes Folder
			std::filesystem::create_directories(scenesDest);
			std::filesystem::copy(scenesSrc, scenesDest, copyOptions);

			// Copy the Asset Folders
			std::filesystem::create_directories(engineAssetDest);
			std::filesystem::copy(engineAssetSrc, engineAssetDest, copyOptions);

			std::filesystem::create_directories(projectAssetDest);
			std::filesystem::copy(projectAssetSrc, projectAssetDest, copyOptions);

			// Create Logs folder
			std::filesystem::create_directories(finalExportPath / "Logs");

			EB_CORE_INFO("Export Complete!");
		}
		catch (const std::filesystem::filesystem_error& e)
		{
			EB_CORE_ERROR("Export failed: {0}", e.what());
			return false;
		}

		return true;
	}

}