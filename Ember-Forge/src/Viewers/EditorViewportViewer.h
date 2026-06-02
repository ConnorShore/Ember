#pragma once

#include "EditorContext.h"
#include <Ember/Core/Time.h>
#include <string>
#include <memory>

namespace Ember {

	class EditorLayer;

	class EditorViewportViewer
	{
	public:
		enum class Type
		{
			Scene = 0,
			Prefab,
			Animation
		};

		EditorViewportViewer(Type type, SharedPtr<Scene> scene, const std::string& filePath, const std::string& title);
		virtual ~EditorViewportViewer() = default;

		Type GetType() const { return m_Type; }
		SharedPtr<Scene> GetScene() const { return m_Scene; }
		const std::string& GetFilePath() const { return m_FilePath; }
		const std::string& GetTitle() const { return m_Title; }

		void SetFilePath(const std::string& filePath) { m_FilePath = filePath; }
		void SetTitle(const std::string& title) { m_Title = title; }

		virtual void OnUpdate(TimeStep delta, EditorLayer* editor) {}
		virtual void OnImGuiRender(EditorLayer* editor) = 0;

		Entity SelectedEntity;
		Entity PreviousSelectedEntity;

	protected:
		Type m_Type;
		SharedPtr<Scene> m_Scene;
		std::string m_FilePath;
		std::string m_Title;
	};

}