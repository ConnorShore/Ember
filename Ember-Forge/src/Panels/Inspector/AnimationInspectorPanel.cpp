
#include "efpch.h"
#include "AnimationInspectorPanel.h"
#include "Viewers/AnimationViewportViewer.h"
#include "UI/PropertyGrid.h"
#include "UI/DragDropTypes.h"

#include <Ember/Asset/AssetManager.h>

namespace Ember {

	AnimationInspectorPanel::AnimationInspectorPanel(EditorContext* context)
		: InspectorPanelContent(context), m_AssetManager(Application::Instance().GetAssetManager())
	{
	}

	AnimationInspectorPanel::~AnimationInspectorPanel()
	{
	}

	void AnimationInspectorPanel::OnImGuiRender()
	{
		auto animationViewport = static_cast<AnimationViewportViewer*>(m_Context->ActiveViewportViewer);
		
		auto selectedState = animationViewport->GetSelectedState();
		auto selectedTransition = animationViewport->GetSelectedTransition();
		if (!selectedState && !selectedTransition)
			return;

		if (selectedState)
			RenderAnimationState(selectedState);

		if (selectedTransition)
			RenderAnimationTransition(selectedTransition);
	}

	void AnimationInspectorPanel::RenderAnimationState(AnimationState* animState)
	{
		std::string nodeLabel = std::format("Animation State: {}###AnimStateNode_{}", animState->Name, animState->Id);
		if (UI::Nodes::BeginExpandableNode(nodeLabel))
		{
			if (UI::PropertyGrid::Begin("AnimStateProps"))
			{
				if (UI::PropertyGrid::InputText("Name", animState->Name))
				{
					auto animationViewport = static_cast<AnimationViewportViewer*>(m_Context->ActiveViewportViewer);
					animationViewport->RenameNode(animState->Id, animState->Name);
				}

				bool animExists = animState->AnimationHandle != Constants::InvalidUUID;
				std::string animName = "None (Animation)";
				if (animExists)
				{
					auto animAsset = m_AssetManager.GetAsset<Animation>(animState->AnimationHandle);
					if (animAsset)
						animName = std::filesystem::path(animAsset->GetFilePath()).filename().string();
				}

				std::string payloadType = DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::AssetAnimation);
				std::string droppedPath;

				auto browseFunc = [&]() {
					ImGui::OpenPopup("ChooseAnimPopup");
					};

				auto clearFunc = animExists ? UI::UICallbackFunc([&]() {
					animState->AnimationHandle = Constants::InvalidUUID;
					}) : nullptr;

				if (UI::PropertyGrid::AssetReference("Animation", animName, payloadType, droppedPath, browseFunc, clearFunc))
				{
					auto animation = m_AssetManager.Load<Animation>(droppedPath);
					if (animation)
					{
						animState->AnimationHandle = animation->GetUUID();
					}
				}

				UI::PropertyGrid::Checkbox("Looping", animState->Looping);
				UI::PropertyGrid::Float("Base Playback Speed", animState->BasePlaybackSpeed);

				UI::PropertyGrid::End();
			}

			UI::Nodes::EndExpandableNode();
		}
	}

	void AnimationInspectorPanel::RenderAnimationTransition(AnimationTransition* animTransition)
	{
		std::string message = std::format("Selected transition from id {} to id {}", animTransition->FromStateId, animTransition->ToStateId);
		ImGui::Text(message.c_str());

		// TODO: If two way transition exists, show both directions as separate "expandable nodes" in the inspector with their own conditions and properties
	}

}