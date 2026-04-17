#include "ebpch.h"
#include "ScriptBindScene.h"

#include "Ember/Scene/Scene.h"
#include "Ember/Math/Math.h"

#include "Ember/Core/Application.h"

namespace Ember {

	void BindScene(sol::state& state, Scene* scene)
	{
		auto sceneTable = state.create_table("Scene");
		sceneTable.set_function("InstantiatePrefab", [scene](const std::string& assetName, const Vector3f& position) {
			auto prefabAsset = Application::Instance().GetAssetManager().GetAsset<Prefab>(assetName);
			return scene->InstantiatePrefab(prefabAsset, &position);
		});
	}

}