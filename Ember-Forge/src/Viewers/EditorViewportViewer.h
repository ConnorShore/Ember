#pragma once

#include "Undo/UndoStack.h"

#include <Ember/Scene/Entity.h>

#include <string>
#include <vector>

namespace Ember {

	class EditorLayer;
	class Scene;

	class EditorViewportViewer
	{
	public:
		enum class Type
		{
			None = 0,
			Scene,
			Prefab,
			Animation,
			SkeletonMask
		};

		EditorViewportViewer(Type type, SharedPtr<Scene> scene, const std::string& filePath, const std::string& title);
		virtual ~EditorViewportViewer() = default;

		Type GetType() const { return m_Type; }
		SharedPtr<Scene> GetScene() const { return m_Scene; }
		const std::string& GetFilePath() const { return m_FilePath; }
		const std::string& GetTitle() const { return m_Title; }

		void SetFilePath(const std::string& filePath) { m_FilePath = filePath; }
		void SetTitle(const std::string& title) { m_Title = title; }

		virtual void OnOpen(EditorLayer* editor) {}
		virtual void OnUpdate(TimeStep delta, EditorLayer* editor) {}
		virtual void OnImGuiRender(EditorLayer* editor) = 0;

		// History is per tab, so a scene and a prefab being edited side by side never share a stack;
		// closing the tab discards it.
		UndoStack& GetUndoStack() { return m_UndoStack; }

		// Per-tab selection, restored when the tab is activated again.
		std::vector<Entity> Selection;
		Entity SelectedEntity;
		Entity PreviousSelectedEntity;

	protected:
		UndoStack m_UndoStack;

		Type m_Type;
		SharedPtr<Scene> m_Scene;
		std::string m_FilePath;
		std::string m_Title;
	};

}