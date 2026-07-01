#pragma once

#include "EditorViewportViewer.h"

namespace Ember {

	class Scene;
	class Prefab;
	class Entity;

	class PrefabViewportViewer final : public EditorViewportViewer
	{
	public:
		PrefabViewportViewer(SharedPtr<Scene> scene, SharedPtr<Prefab> prefab, Entity rootEntity, const std::string& filePath, const std::string& title);

		virtual void OnOpen(EditorLayer* editor) override;
		virtual void OnImGuiRender(EditorLayer* editor) override;

		void RefreshPrefab();

		SharedPtr<Prefab> PrefabAsset;
		Entity RootEntity;
	};

}