#include "ebpch.h"
#include "ScriptBindScene.h"

#include "Ember/Scene/Scene.h"
#include "Ember/Scene/SceneManager.h"
#include "Ember/Math/Math.h"

#include "Ember/Core/Application.h"
#include "Ember/Core/ProjectManager.h"

namespace Ember {

	void BindScene(sol::state& state, Scene* scene)
	{
		auto sceneTable = state.create_table("Scene");

		// Entity
		sceneTable.set_function("AddEntity", sol::overload(
			[scene](const std::string& name) {
				return scene->AddEntity(name);
			}
		));
		sceneTable.set_function("RemoveEntity", sol::overload(
			[scene](Entity entity) {
				scene->RemoveEntity(entity);
			},
			[scene](EntityID entityID) {
				Entity entity(entityID, scene);
				scene->RemoveEntity(entity);
			}
		));
		sceneTable.set_function("DuplicateEntity", [scene](const std::string& name) {
			Entity entity = scene->GetEntity(name);
			return scene->DuplicateEntity(entity);
		});
		sceneTable.set_function("GetEntityByName", [scene](const std::string& name) {
			return scene->GetEntity(name);
		});

		// Prefab
		sceneTable.set_function("InstantiatePrefab", sol::overload(
			[scene](const std::string& assetName, const Vector3f& position) {
				auto prefabAsset = Application::Instance().GetAssetManager().GetAsset<Prefab>(assetName);
				return scene->InstantiatePrefab(prefabAsset, &position);
			},
			[scene](const std::string& assetName, Entity parent) {
				auto prefabAsset = Application::Instance().GetAssetManager().GetAsset<Prefab>(assetName);
				Vector3f pos(0.0f);
				return scene->InstantiatePrefab(prefabAsset, parent, &pos);
			},
			[scene](const std::string& assetName, Entity parent, const Vector3f& position) {
				auto prefabAsset = Application::Instance().GetAssetManager().GetAsset<Prefab>(assetName);
				return scene->InstantiatePrefab(prefabAsset, parent, &position);
			}
		));

		// Camera
		sceneTable.set_function("SetActiveCamera", [scene](const std::string& entityName) {
			Entity cameraEntity = scene->GetEntity(entityName);
			scene->SetActiveCamera(cameraEntity);
		});

		// Pools
		sceneTable.set_function("CreatePool", [scene](const std::string& poolID, const std::string& prefabName, uint32_t initialSize) {
			auto prefab = Application::Instance().GetAssetManager().GetAsset<Prefab>(prefabName);
			scene->GetPoolManager().CreatePool(scene, poolID, prefab->GetUUID(), initialSize);
		});

		sceneTable.set_function("RetrieveFromPool", sol::overload(
			[scene](const std::string& poolID) {
				return scene->GetPoolManager().RetrieveFromPool(scene, poolID);
			},
			[scene](const std::string& poolID, const Vector3f& position) {
				return scene->GetPoolManager().RetrieveFromPool(scene, poolID, position);
			}
		));

		// Scene transitions - queues a deferred load so the current frame finishes safely
		auto sceneManagerTable = state.create_table("SceneManager");
		sceneManagerTable.set_function("LoadScene", [](const std::string& name) {
			auto sceneAsset = Application::Instance().GetAssetManager().GetAsset<Scene>(name);
			if (!sceneAsset)
			{
				EB_CORE_ERROR("Attempted to load scene with name \"{}\" but it doesn't exist!", name);
				return;
			}

			Application::Instance().GetSceneManager().LoadScene(sceneAsset->GetFilePath());
		});
		sceneManagerTable.set_function("LoadNextScene", []() {
			UUID nextSceneUUID = ProjectManager::GetActive()->GetNextScene();
			if (nextSceneUUID != Constants::InvalidUUID)
				Application::Instance().GetSceneManager().LoadScene(nextSceneUUID);
		});
		sceneManagerTable.set_function("LoadDefaultScene", []() {
			UUID defaultSceneUUID = ProjectManager::GetActive()->GetScenesInBuild()[0];
			if (defaultSceneUUID != Constants::InvalidUUID)
				Application::Instance().GetSceneManager().LoadScene(defaultSceneUUID);
		});
		sceneManagerTable.set_function("IsLastScene", []() {
			return ProjectManager::GetActive()->IsLastScene();
		});
	}
}