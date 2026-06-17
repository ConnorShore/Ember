#include "efpch.h"
#include "SkeletonMaskViewportViewer.h"
#include "EditorLayer.h"

#include "UI/PropertyGrid.h"
#include "UI/DragDropTypes.h"
#include <Ember/Asset/AssetManager.h>
#include <Ember/Asset/SkeletonMaskSerializer.h>
#include <Ember/Event/UIEvent.h>

#include <imgui/imgui.h>

namespace Ember {

	SkeletonMaskViewportViewer::SkeletonMaskViewportViewer(SharedPtr<Scene> scene, SharedPtr<SkeletonMask> skeletonMask, const std::string& filePath, const std::string& title)
		: EditorViewportViewer(Type::SkeletonMask, scene, filePath, title), m_SkeletonMask(skeletonMask)
	{
	}

	SkeletonMaskViewportViewer::~SkeletonMaskViewportViewer()
	{
	}

	void SkeletonMaskViewportViewer::BuildHierarchyCache()
	{
		m_RootBones.clear();
		m_BoneChildrenMap.clear();

		auto skeleton = m_SkeletonMask->GetSkeleton();
		if (!skeleton)
		{
			m_CachedSkeletonHandle = Constants::InvalidUUID;
			return;
		}

		m_CachedSkeletonHandle = skeleton->GetUUID();
		const auto& bones = skeleton->GetBones();
		m_BoneChildrenMap.resize(bones.size());

		for (uint32_t i = 0; i < bones.size(); ++i)
		{
			uint32_t parentID = bones[i].ParentID;

			// If ParentID is out of bounds or points to itself, it's a root bone
			if (parentID >= bones.size() || parentID == i)
			{
				m_RootBones.push_back(i);
			}
			else
			{
				m_BoneChildrenMap[parentID].push_back(i);
			}
		}
	}

	void SkeletonMaskViewportViewer::OnImGuiRender(EditorLayer* editor)
	{
		editor->SetViewportHovered(ImGui::IsWindowHovered());
		editor->SetViewportFocused(ImGui::IsWindowFocused());

		// Check if the skeleton has changed, and rebuild our tree cache if so
		UUID currentSkeletonId = m_SkeletonMask->GetSkeleton() ? m_SkeletonMask->GetSkeleton()->GetUUID() : (UUID)Constants::InvalidUUID;
		if (currentSkeletonId != m_CachedSkeletonHandle)
		{
			BuildHierarchyCache();
		}

		// CHANGED: Now calculates exactly 1/2 of the screen width for the left panel
		float leftPanelWidth = ImGui::GetContentRegionAvail().x * 0.5f;

		// ==========================================================
		// --- LEFT PANEL: Skeleton Mask Hierarchy ---
		// ==========================================================
		if (ImGui::BeginChild("MaskLeftPanel", ImVec2(leftPanelWidth, 0), true))
		{
			DrawLeftPanel();
		}
		ImGui::EndChild();

		ImGui::SameLine();

		// ==========================================================
		// --- RIGHT PANEL: 3D Viewport ---
		// ==========================================================
		if (ImGui::BeginChild("MaskRightPanel", ImVec2(0, 0), true))
		{
			DrawRightPanel();
		}
		ImGui::EndChild();
	}

	void SkeletonMaskViewportViewer::DrawLeftPanel()
	{
		// 1. Skeleton Selector
		if (UI::PropertyGrid::Begin("SkeletonAssignment"))
		{
			bool hasSkeleton = m_SkeletonMask->GetSkeleton() != nullptr;
			std::string skelName = "None (Skeleton)";

			if (hasSkeleton)
				skelName = std::filesystem::path(m_SkeletonMask->GetSkeleton()->GetFilePath()).filename().string();

			std::string payloadType = DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::AssetSkeleton); // Assuming this enum exists
			std::string droppedPath;

			auto clearFunc = hasSkeleton ? UI::UICallbackFunc([&]() { m_SkeletonMask->SetSkeleton(nullptr); m_IsDirty = true; }) : nullptr;

			if (UI::PropertyGrid::AssetReference("Target Skeleton", skelName, payloadType, droppedPath, nullptr, clearFunc))
			{
				if (auto skeletonAsset = Application::Instance().GetAssetManager().Load<Skeleton>(droppedPath))
				{
					m_SkeletonMask->SetSkeleton(skeletonAsset);
					m_IsDirty = true;
				}
			}
			UI::PropertyGrid::End();
		}

		ImGui::Separator();
		ImGui::Spacing();

		if (!m_SkeletonMask->GetSkeleton())
		{
			ImGui::TextDisabled("Assign a Skeleton to edit the mask.");
			return;
		}

		ImGui::Text("Bone Weights");
		ImGui::TextDisabled("(Hold CTRL while editing to apply to all children)");
		ImGui::Spacing();

		// 2. The Hierarchical Table
		ImGuiTableFlags flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody;

		if (ImGui::BeginTable("BoneTreeTable", 2, flags))
		{
			ImGui::TableSetupColumn("Bone Hierarchy", ImGuiTableColumnFlags_WidthStretch, 0.65f);
			ImGui::TableSetupColumn("Weight", ImGuiTableColumnFlags_WidthStretch, 0.35f);
			ImGui::TableHeadersRow();

			for (uint32_t rootBoneIdx : m_RootBones)
			{
				DrawBoneNode(rootBoneIdx);
			}

			ImGui::EndTable();
		}
	}

	void SkeletonMaskViewportViewer::DrawBoneNode(uint32_t boneIndex)
	{
		auto skeleton = m_SkeletonMask->GetSkeleton();
		const auto& bones = skeleton->GetBones();
		const Bone& bone = bones[boneIndex];
		const auto& children = m_BoneChildrenMap[boneIndex];

		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		bool isLeaf = children.empty();

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_DefaultOpen;
		if (isLeaf) flags |= ImGuiTreeNodeFlags_Leaf;

		ImGui::PushID(boneIndex);

		float currentWeight = m_SkeletonMask->GetBoneWeight(bone);
		bool isChecked = currentWeight > 0.0f; // Visual toggle state

		// Temporarily make the tree node's hover and active background colors completely transparent
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

		ImGui::SetNextItemAllowOverlap();
		bool isOpen = ImGui::TreeNodeEx("##node", flags);

		// Pop the transparent colors immediately so we don't break the rest of the UI
		ImGui::PopStyleColor(2);

		// Insert the checkbox and bone name seamlessly next to the arrow
		ImGui::SameLine();
		bool checkboxToggled = ImGui::Checkbox("##check", &isChecked);

		ImGui::SameLine();
		ImGui::TextUnformatted(bone.Name.c_str());

		// Move to the second column for the slider
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-FLT_MIN);
		bool sliderChanged = ImGui::SliderFloat("##weight", &currentWeight, 0.0f, 1.0f, "%.2f");

		// Handle data updates
		if (checkboxToggled)
		{
			currentWeight = isChecked ? 1.0f : 0.0f;
			m_SkeletonMask->SetBoneWeight(bone, currentWeight);
			m_IsDirty = true;

			if (ImGui::GetIO().KeyCtrl)
				ApplyWeightToBoneAndChildren(boneIndex, currentWeight);
		}
		else if (sliderChanged)
		{
			m_SkeletonMask->SetBoneWeight(bone, currentWeight);
			m_IsDirty = true;

			if (ImGui::GetIO().KeyCtrl)
				ApplyWeightToBoneAndChildren(boneIndex, currentWeight);
		}

		// Recurse children
		if (isOpen)
		{
			for (uint32_t childIdx : children)
			{
				DrawBoneNode(childIdx);
			}
			ImGui::TreePop();
		}

		ImGui::PopID();
	}

	void SkeletonMaskViewportViewer::ApplyWeightToBoneAndChildren(uint32_t boneIndex, float weight)
	{
		auto skeleton = m_SkeletonMask->GetSkeleton();
		const auto& bones = skeleton->GetBones();

		m_SkeletonMask->SetBoneWeight(bones[boneIndex], weight);

		for (uint32_t childIndex : m_BoneChildrenMap[boneIndex])
		{
			ApplyWeightToBoneAndChildren(childIndex, weight);
		}
	}

	void SkeletonMaskViewportViewer::SaveSkeletonMask(EditorLayer* editor)
	{
		SkeletonMaskSerializer serializer;
		if (serializer.Serialize(GetFilePath(), m_SkeletonMask))
		{
			m_IsDirty = false;
			auto evt = UINotificationEvent("Skeleton Mask saved successfully", UINotificationEvent::Severity::Info);
			editor->GetContext().EventCallback(evt);
		}
		else
		{
			auto evt = UINotificationEvent("Failed to save Skeleton Mask", UINotificationEvent::Severity::Error);
			editor->GetContext().EventCallback(evt);
		}
	}

	void SkeletonMaskViewportViewer::DrawRightPanel()
	{
		ImGui::TextDisabled("3D Viewport Placeholder");
		ImGui::TextWrapped("Once rendering is set up, the skeleton can be rendered here. Bones with a weight of 1.0 can be tinted white, and 0.0 tinted dark gray to visualize the mask.");
	}

}