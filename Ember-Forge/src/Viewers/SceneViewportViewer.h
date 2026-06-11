#pragma once

#include "EditorViewportViewer.h"

namespace Ember {

	class SceneViewportViewer final : public EditorViewportViewer
	{
	public:
		SceneViewportViewer(SharedPtr<Scene> scene, const std::string& filePath, const std::string& title);
		virtual void OnImGuiRender(EditorLayer* editor) override;
	};

}