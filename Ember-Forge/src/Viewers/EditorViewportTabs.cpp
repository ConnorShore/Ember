#include "efpch.h"
#include "EditorViewportTabs.h"
#include "EditorLayer.h"

#include <Ember/Animation/AnimationStateMachine.h>

namespace Ember {

	std::filesystem::path EditorViewportTabs::NormalizedPath(const std::string& path)
	{
		return std::filesystem::absolute(std::filesystem::path(path)).lexically_normal();
	}

	bool EditorViewportTabs::PathsMatch(const std::string& lhs, const std::string& rhs)
	{
		if (lhs.empty() || rhs.empty())
			return false;

		std::error_code ec;
		if (std::filesystem::equivalent(lhs, rhs, ec))
			return true;

		return NormalizedPath(lhs) == NormalizedPath(rhs);
	}

	std::string EditorViewportTabs::TitleFromPath(const std::string& path, const std::string& fallback)
	{
		if (path.empty())
			return fallback;

		std::string title = std::filesystem::path(path).stem().string();
		return title.empty() ? fallback : title;
	}

	void EditorViewportTabs::OnUpdate(TimeStep delta, EditorLayer* editor)
	{
		//for (size_t viewerIndex = 0; viewerIndex < m_Viewers.size(); viewerIndex++)
		//{
		//	auto& viewer = *m_Viewers[viewerIndex];
		//	viewer.OnUpdate(delta, editor);
		//}

		// Update active viewer tab only
		if (m_ActiveViewerIndex < 0 || m_ActiveViewerIndex >= m_Viewers.size())
			return;

		m_Viewers[m_ActiveViewerIndex]->OnUpdate(delta, editor);
	}

	bool EditorViewportTabs::Render(EditorLayer* editor, const ActivateViewerCallback& activateViewer, const CloseViewerCallback& closeViewer)
	{
		ImGuiWindowClass windowClass;
		windowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;
		ImGui::SetNextWindowClass(&windowClass);

		ImGui::Begin("Scene");

		if (m_Viewers.empty())
		{
			//editor->GetContext().ActiveViewportType = EditorViewportViewer::Type::None;
			editor->GetContext().ActiveViewportViewer = nullptr;

			ImGui::TextDisabled("No scene or prefab open.");
			ImGui::End();
			return false;
		}

		int pendingCloseIndex = -1;
		bool renderedActiveTab = false;

		if (ImGui::BeginTabBar("SceneViewerTabs", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_AutoSelectNewTabs))
		{
			for (size_t viewerIndex = 0; viewerIndex < m_Viewers.size(); viewerIndex++)
			{
				auto& viewer = *m_Viewers[viewerIndex];
				bool tabOpen = true;
				ImGuiTabItemFlags tabFlags = m_SyncActiveViewerSelection && static_cast<int>(viewerIndex) == m_ActiveViewerIndex ? ImGuiTabItemFlags_SetSelected : 0;
				std::string tabLabel = std::format("{}###ViewerTab{}", viewer.GetTitle(), viewer.GetFilePath().empty() ? std::to_string(viewerIndex) : viewer.GetFilePath());

				if (ImGui::BeginTabItem(tabLabel.c_str(), &tabOpen, tabFlags))
				{
					if (static_cast<int>(viewerIndex) != m_ActiveViewerIndex)
						ActivateViewer(viewerIndex, activateViewer);

					if (tabOpen)
					{
						// If first frame tab is opened, call viewer.OnOpen()
						// TODO: Get this working
						if (viewerIndex != m_ActiveTabIndex)
						{
							viewer.OnOpen(editor);
							m_ActiveTabIndex = static_cast<uint8_t>(viewerIndex);
						}

						// Give the concrete viewer control over its own ImGui render pass
						//editor->GetContext().ActiveViewportType = viewer.GetType();
						editor->GetContext().ActiveViewportViewer = &viewer;

						viewer.OnImGuiRender(editor);
						renderedActiveTab = true;
					}

					ImGui::EndTabItem();
				}

				if (!tabOpen && pendingCloseIndex == -1)
					pendingCloseIndex = static_cast<int>(viewerIndex);
			}

			ImGui::EndTabBar();
		}
		m_SyncActiveViewerSelection = false;

		if (pendingCloseIndex >= 0 && CloseViewer(static_cast<size_t>(pendingCloseIndex), true, closeViewer, activateViewer) && m_Viewers.empty())
			renderedActiveTab = false;

		ImGui::End();
		return renderedActiveTab;
	}

	size_t EditorViewportTabs::AddSceneViewer(SharedPtr<Scene> scene, const std::string& filePath, const std::string& title)
	{
		m_Viewers.push_back(ScopedPtr<SceneViewportViewer>::Create(scene, filePath, title));
		return m_Viewers.size() - 1;
	}

	size_t EditorViewportTabs::AddPrefabViewer(SharedPtr<Scene> scene, SharedPtr<Prefab> prefab, Entity rootEntity, const std::string& filePath, const std::string& title)
	{
		m_Viewers.push_back(ScopedPtr<PrefabViewportViewer>::Create(scene, prefab, rootEntity, filePath, title));
		return m_Viewers.size() - 1;
	}

	size_t EditorViewportTabs::AddAnimationViewer(SharedPtr<Scene> scene, SharedPtr<AnimationStateMachine> animationStateMachine, const std::string& filePath, const std::string& title)
	{
		m_Viewers.push_back(ScopedPtr<AnimationViewportViewer>::Create(scene, animationStateMachine, filePath, title));
		return m_Viewers.size() - 1;
	}

	// [The rest of EditorViewportTabs.cpp remains identical for GetActiveViewer, ActivateViewer, CloseViewer, etc.]

	EditorViewportViewer* EditorViewportTabs::GetActiveViewer() {
		if (m_ActiveViewerIndex < 0 || static_cast<size_t>(m_ActiveViewerIndex) >= m_Viewers.size()) return nullptr;
		return m_Viewers[static_cast<size_t>(m_ActiveViewerIndex)].Ptr();
	}

	const EditorViewportViewer* EditorViewportTabs::GetActiveViewer() const {
		if (m_ActiveViewerIndex < 0 || static_cast<size_t>(m_ActiveViewerIndex) >= m_Viewers.size()) return nullptr;
		return m_Viewers[static_cast<size_t>(m_ActiveViewerIndex)].Ptr();
	}

	EditorViewportViewer* EditorViewportTabs::GetViewer(size_t viewerIndex) {
		if (viewerIndex >= m_Viewers.size()) return nullptr;
		return m_Viewers[viewerIndex].Ptr();
	}

	const EditorViewportViewer* EditorViewportTabs::GetViewer(size_t viewerIndex) const {
		if (viewerIndex >= m_Viewers.size()) return nullptr;
		return m_Viewers[viewerIndex].Ptr();
	}

	int EditorViewportTabs::FindViewer(EditorViewportViewer::Type type, const std::string& filePath) const {
		for (size_t viewerIndex = 0; viewerIndex < m_Viewers.size(); viewerIndex++) {
			const auto& viewer = *m_Viewers[viewerIndex];
			if (viewer.GetType() == type && PathsMatch(viewer.GetFilePath(), filePath))
				return static_cast<int>(viewerIndex);
		}
		return -1;
	}

	void EditorViewportTabs::StoreViewerState(size_t viewerIndex, Entity selectedEntity, Entity previousSelectedEntity) {
		if (EditorViewportViewer* viewer = GetViewer(viewerIndex)) {
			viewer->SelectedEntity = selectedEntity;
			viewer->PreviousSelectedEntity = previousSelectedEntity;
		}
	}

	void EditorViewportTabs::StoreActiveViewerState(Entity selectedEntity, Entity previousSelectedEntity) {
		if (m_ActiveViewerIndex < 0) return;
		StoreViewerState(static_cast<size_t>(m_ActiveViewerIndex), selectedEntity, previousSelectedEntity);
	}

	void EditorViewportTabs::ActivateViewer(size_t viewerIndex, const ActivateViewerCallback& activateViewer) {
		if (viewerIndex >= m_Viewers.size())
			return;

		size_t previousViewerIndex = m_ActiveViewerIndex >= 0 ? static_cast<size_t>(m_ActiveViewerIndex) : static_cast<size_t>(-1);
		m_ActiveViewerIndex = static_cast<int>(viewerIndex);
		m_SyncActiveViewerSelection = true;
		activateViewer(previousViewerIndex, viewerIndex, *m_Viewers[viewerIndex]);
	}

	bool EditorViewportTabs::CloseViewer(size_t viewerIndex, bool saveBeforeClose, const CloseViewerCallback& closeViewer, const ActivateViewerCallback& activateViewer) {
		if (viewerIndex >= m_Viewers.size())
			return true;

		if (!closeViewer(viewerIndex, *m_Viewers[viewerIndex], saveBeforeClose))
			return false;

		bool closedActiveViewer = static_cast<int>(viewerIndex) == m_ActiveViewerIndex;
		m_Viewers.erase(m_Viewers.begin() + static_cast<std::ptrdiff_t>(viewerIndex));
		if (m_Viewers.empty()) 
		{ 
			m_ActiveViewerIndex = -1;
			return true; 
		}

		if (closedActiveViewer) 
		{
			m_ActiveViewerIndex = -1;
			ActivateViewer(std::min(viewerIndex, m_Viewers.size() - 1), activateViewer);
		}
		else if (static_cast<int>(viewerIndex) < m_ActiveViewerIndex)
			m_ActiveViewerIndex--;

		return true;
	}

	bool EditorViewportTabs::CloseAllViewers(bool savePrefabs, const CloseViewerCallback& closeViewer, const ActivateViewerCallback& activateViewer) {
		while (!m_Viewers.empty()) { if (!CloseViewer(0, savePrefabs, closeViewer, activateViewer)) return false; }
		return true;
	}

	void EditorViewportTabs::Clear() {
		m_Viewers.clear(); m_ActiveViewerIndex = -1; m_SyncActiveViewerSelection = false;
	}
}