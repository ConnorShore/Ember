#pragma once

#include "EditorViewportViewer.h"
#include "SceneViewportViewer.h"
#include "PrefabViewportViewer.h"
#include "AnimationViewportViewer.h"

#include <functional>
#include <string>
#include <vector>
#include <filesystem>

namespace Ember {

	class AnimationController;
	class Prefab;
	class Entity;
	class Scene;
	class EditorLayer;

	class EditorViewportTabs
	{
	public:
		using ActivateViewerCallback = std::function<void(size_t previousViewerIndex, size_t activeViewerIndex, EditorViewportViewer& activeViewer)>;
		using CloseViewerCallback = std::function<bool(size_t viewerIndex, EditorViewportViewer& viewer, bool saveBeforeClose)>;

		static std::filesystem::path NormalizedPath(const std::string& path);
		static bool PathsMatch(const std::string& lhs, const std::string& rhs);
		static std::string TitleFromPath(const std::string& path, const std::string& fallback);

		// Pass EditorLayer so it can be forwarded to the concrete Viewers
		void OnUpdate(TimeStep delta, EditorLayer* editor);
		bool Render(EditorLayer* editor, const ActivateViewerCallback& activateViewer, const CloseViewerCallback& closeViewer);

		size_t AddSceneViewer(SharedPtr<Scene> scene, const std::string& filePath, const std::string& title);
		size_t AddPrefabViewer(SharedPtr<Scene> scene, SharedPtr<Prefab> prefab, Entity rootEntity, const std::string& filePath, const std::string& title);
		size_t AddAnimationViewer(SharedPtr<Scene> scene, SharedPtr<AnimationController> animationController, const std::string& filePath, const std::string& title);

		EditorViewportViewer* GetActiveViewer();
		const EditorViewportViewer* GetActiveViewer() const;
		EditorViewportViewer* GetViewer(size_t viewerIndex);
		const EditorViewportViewer* GetViewer(size_t viewerIndex) const;

		int GetActiveViewerIndex() const { return m_ActiveViewerIndex; }
		bool Empty() const { return m_Viewers.empty(); }
		int FindViewer(EditorViewportViewer::Type type, const std::string& filePath) const;

		void StoreViewerState(size_t viewerIndex, Entity selectedEntity, Entity previousSelectedEntity);
		void StoreActiveViewerState(Entity selectedEntity, Entity previousSelectedEntity);

		void ActivateViewer(size_t viewerIndex, const ActivateViewerCallback& activateViewer);
		bool CloseViewer(size_t viewerIndex, bool saveBeforeClose, const CloseViewerCallback& closeViewer, const ActivateViewerCallback& activateViewer);
		bool CloseAllViewers(bool savePrefabs, const CloseViewerCallback& closeViewer, const ActivateViewerCallback& activateViewer);

		void Clear();

	private:
		std::vector<ScopedPtr<EditorViewportViewer>> m_Viewers;
		int m_ActiveViewerIndex = -1;
		bool m_SyncActiveViewerSelection = false;
		uint8_t m_ActiveTabIndex = 0;
	};
}