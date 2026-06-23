#pragma once

#include "EditorViewportViewer.h"
#include <Ember/Asset/SkeletonMask.h>
#include <vector>

namespace Ember {

	class SkeletonMaskViewportViewer final : public EditorViewportViewer
	{
	public:
		SkeletonMaskViewportViewer(SharedPtr<Scene> scene, SharedPtr<SkeletonMask> skeletonMask, const std::string& filePath, const std::string& title);
		virtual ~SkeletonMaskViewportViewer();

		virtual void OnImGuiRender(EditorLayer* editor) override;

		void SaveSkeletonMask(EditorLayer* editor);

	private:
		void DrawLeftPanel();
		void DrawRightPanel();

		void BuildHierarchyCache();
		void DrawBoneNode(uint32_t boneIndex);
		void ApplyWeightToBoneAndChildren(uint32_t boneIndex, float weight);

	private:
		SharedPtr<SkeletonMask> m_SkeletonMask;

		// Caches for traversing the flat skeleton array as a hierarchy
		UUID m_CachedSkeletonHandle = Constants::InvalidUUID;
		std::vector<uint32_t> m_RootBones;
		std::vector<std::vector<uint32_t>> m_BoneChildrenMap;

		bool m_IsDirty = false;
	};
}