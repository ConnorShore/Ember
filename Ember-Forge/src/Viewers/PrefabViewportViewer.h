#pragma once

#include "EditorViewportViewer.h"

namespace Ember {

	class PrefabViewportViewer final : public EditorViewportViewer
	{
	public:
		PrefabViewportViewer(SharedPtr<Scene> scene, SharedPtr<Prefab> prefab, Entity rootEntity, const std::string& filePath, const std::string& title);
		virtual void OnImGuiRender(EditorLayer* editor) override;

		SharedPtr<Prefab> PrefabAsset;
		Entity RootEntity;
	};

}