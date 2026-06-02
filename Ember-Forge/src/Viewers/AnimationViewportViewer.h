#pragma once

#include "EditorViewportViewer.h"

namespace Ember {

	class AnimationViewportViewer final : public EditorViewportViewer
	{
	public:
		AnimationViewportViewer(SharedPtr<Scene> scene, const std::string& filePath, const std::string& title);
		virtual void OnImGuiRender(EditorLayer* editor) override;
	};

}