#pragma once

#include "ComponentUI.h"
#include "Ui/PropertyGrid.h"
#include "UI/DragDropTypes.h"

#include "Ember/Asset/Font.h"
#include "Ember/Utils/PlatformUtil.h"

#include <imgui/imgui.h>

namespace Ember {

	class AudioSourceComponentUI : public ComponentUI<AudioSourceComponent>
	{
	public:
		AudioSourceComponentUI(EditorContext* context) : ComponentUI(context) { m_CanRemove = false; }
		inline const char* GetName() const override { return "Audio Source Component"; }

	protected:
		inline void RenderComponentImpl(AudioSourceComponent& component) override
		{
			if (UI::PropertyGrid::Begin("AudioSourceSourceProps"))
			{
				RenderAudioClipSelector(component);
				UI::PropertyGrid::End();
			}

			ImGui::Separator();

			ImGui::TextDisabled("Properties");
			if (UI::PropertyGrid::Begin("AudioSourceProps"))
			{
				UI::PropertyGrid::Float("Volume", component.Properties.Volume, 0.01f);
				UI::PropertyGrid::Float("Pitch", component.Properties.Pitch, 0.01f);
				UI::PropertyGrid::Checkbox("Looping", component.Properties.Looping);
				UI::PropertyGrid::Checkbox("Spatialized", component.Properties.Spatialized);

				UI::PropertyGrid::End();
			}
		}

	private:
		void RenderAudioClipSelector(AudioSourceComponent& component)
		{
			//auto& assetManager = Application::Instance().GetAssetManager();
			//std::string selectedAudioClip;
			//bool clipExists = component.AudioClipHandle != Constants::InvalidUUID;
			//if (clipExists)
			//{
			//	auto audioAsset = assetManager.GetAsset<AudioClip>(component.AudioClipHandle);
			//	if (audioAsset)
			//	{
			//		UI::PropertyGrid::AssetReference("Audio Clip", audioAsset->GetName(), DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::AssetAudioClip),
			//			selectedAudioClip, nullptr, nullptr);
			//	}
			//}
			//else
			//{
			//	UI::PropertyGrid::AssetReference("Audio Clip", "None (Audio Clip)", DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::AssetAudioClip),
			//		selectedAudioClip, nullptr, nullptr);
			//}
			//if (!selectedAudioClip.empty())
			//{
			//	auto audioAsset = assetManager.GetAssetByPath<AudioClip>(selectedAudioClip);
			//	if (audioAsset != nullptr)
			//		component.AudioClipHandle = audioAsset->GetUUID();
			//	else
			//	{
			//		auto audioAsset = assetManager.Load<AudioClip>(selectedAudioClip);
			//		component.AudioClipHandle = audioAsset ? audioAsset->GetUUID() : (UUID)Constants::InvalidUUID;
			//	}
			//}

			
			// Audio asset selector
			auto& assetManager = Application::Instance().GetAssetManager();

			auto chooseClipFunc = [&]() {
				ImGui::OpenPopup("ChooseAudioClipPopup");
				};
			auto clearClipFunc = UI::UICallbackFunc([&]() {
				component.AudioClipHandle = Constants::InvalidUUID;
				});

			std::string selectedAudioClip;
			bool clipExists = component.AudioClipHandle != Constants::InvalidUUID;
			if (clipExists)
			{
				auto audioAsset = assetManager.GetAsset<AudioClip>(component.AudioClipHandle);
				if (audioAsset)
				{
					UI::PropertyGrid::AssetReference("Audio Clip", audioAsset->GetName(), DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::AssetAudioClip),
						selectedAudioClip, chooseClipFunc, clearClipFunc);
				}
			}
			else
			{
				UI::PropertyGrid::AssetReference("Audio Clip", "None (Audio Clip)", DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::AssetAudioClip),
					selectedAudioClip, chooseClipFunc, nullptr);
			}

			// Set component handle
			if (!selectedAudioClip.empty())
			{
				auto audioAsset = assetManager.GetAssetByPath<AudioClip>(selectedAudioClip);
				if (audioAsset != nullptr)
					component.AudioClipHandle = audioAsset->GetUUID();
				else
				{
					auto audioAsset = assetManager.Load<AudioClip>(selectedAudioClip);
					component.AudioClipHandle = audioAsset ? audioAsset->GetUUID() : (UUID)Constants::InvalidUUID;
				}
			}

			if (ImGui::BeginPopup("ChooseAudioClipPopup"))
			{
				if (ImGui::MenuItem("Load from file..."))
				{
					std::string fileTypes = DragDropUtils::DragDropPayloadTypeToExtension(DragDropPayloadType::AssetAudioClip);
					std::string audioFile = FileDialog::OpenFile("Ember-Forge/assets/audio", std::format("Audio ({})", fileTypes).c_str(), fileTypes.c_str());
					if (!audioFile.empty())
					{
						auto audioAsset = assetManager.Load<AudioClip>(audioFile);
						if (audioAsset)
							component.AudioClipHandle = audioAsset->GetUUID();
					}
				}

				ImGui::EndPopup();
			}
			
		}
	};

}