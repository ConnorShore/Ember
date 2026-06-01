#pragma once

#include "EditorContext.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <filesystem>

namespace Ember {

	class EditorViewportViewer
	{
	public:
		enum class Type
		{
			Scene = 0,
			Prefab
		};

		EditorViewportViewer(Type type, SharedPtr<Scene> scene, const std::string& filePath, const std::string& title);
		virtual ~EditorViewportViewer() = default;

		Type GetType() const { return m_Type; }
		SharedPtr<Scene> GetScene() const { return m_Scene; }
		const std::string& GetFilePath() const { return m_FilePath; }
		const std::string& GetTitle() const { return m_Title; }

		void SetFilePath(const std::string& filePath) { m_FilePath = filePath; }
		void SetTitle(const std::string& title) { m_Title = title; }

		Entity SelectedEntity;
		Entity PreviousSelectedEntity;

	private:
		Type m_Type;
		SharedPtr<Scene> m_Scene;
		std::string m_FilePath;
		std::string m_Title;
	};

	class SceneViewportViewer final : public EditorViewportViewer
	{
	public:
		SceneViewportViewer(SharedPtr<Scene> scene, const std::string& filePath, const std::string& title);
	};

	class PrefabViewportViewer final : public EditorViewportViewer
	{
	public:
		PrefabViewportViewer(SharedPtr<Scene> scene, SharedPtr<Prefab> prefab, Entity rootEntity, const std::string& filePath, const std::string& title);

		SharedPtr<Prefab> PrefabAsset;
		Entity RootEntity;
	};

	class EditorViewportTabs
	{
	public:
		using RenderActiveViewerCallback = std::function<void()>;
		using ActivateViewerCallback = std::function<void(size_t previousViewerIndex, size_t activeViewerIndex, EditorViewportViewer& activeViewer)>;
		using CloseViewerCallback = std::function<bool(size_t viewerIndex, EditorViewportViewer& viewer, bool saveBeforeClose)>;

		static std::filesystem::path NormalizedPath(const std::string& path);
		static bool PathsMatch(const std::string& lhs, const std::string& rhs);
		static std::string TitleFromPath(const std::string& path, const std::string& fallback);

		bool Render(const RenderActiveViewerCallback& renderActiveViewer, const ActivateViewerCallback& activateViewer, const CloseViewerCallback& closeViewer);

		size_t AddSceneViewer(SharedPtr<Scene> scene, const std::string& filePath, const std::string& title);
		size_t AddPrefabViewer(SharedPtr<Scene> scene, SharedPtr<Prefab> prefab, Entity rootEntity, const std::string& filePath, const std::string& title);

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
		std::vector<std::unique_ptr<EditorViewportViewer>> m_Viewers;
		int m_ActiveViewerIndex = -1;
		bool m_SyncActiveViewerSelection = false;
	};

}