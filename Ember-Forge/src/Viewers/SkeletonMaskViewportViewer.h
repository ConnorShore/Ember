#pragma once

#include "EditorViewportViewer.h"

namespace Ember {

	class SkeletonMask;

	class SkeletonMaskViewportViewer final : public EditorViewportViewer
	{
	public:
		SkeletonMaskViewportViewer(SharedPtr<Scene> scene, SharedPtr<SkeletonMask> skeletonMask, const std::string& filePath, const std::string& title);
		virtual ~SkeletonMaskViewportViewer();

		virtual void OnImGuiRender(EditorLayer* editor) override;

	private:
		SharedPtr<SkeletonMask> m_SkeletonMask;
	};
}