#include "ebpch.h"
#include "ScriptBindScene.h"

#include "Ember/Scene/Scene.h"
#include "Ember/Math/Math.h"

#include "Ember/Core/Application.h"

namespace Ember {

	void BindScene(sol::state& state, Scene* scene)
	{
		auto sceneTable = state.create_table("Scene");

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

		sceneTable.set_function("InstantiatePrefab", [scene](const std::string& assetName, const Vector3f& position) {
			auto prefabAsset = Application::Instance().GetAssetManager().GetAsset<Prefab>(assetName);
			return scene->InstantiatePrefab(prefabAsset, &position);
		});

		sceneTable.set_function("SetActiveCamera", [scene](const std::string& entityName) {
			Entity cameraEntity = scene->GetEntity(entityName);
			scene->SetActiveCamera(cameraEntity);
		});
	}
}