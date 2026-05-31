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

				// Spatialization properties
				if (component.Properties.Spatialized)
				{
					if (UI::PropertyGrid::Float("Min Distance", component.Properties.MinDistance, 0.1f, 0.01f))
					{
						// Ensure MinDistance is > 0.0f to avoid issues with ma_sound_set_min_distance
						if (component.Properties.MinDistance <= 0.0f)
							component.Properties.MinDistance = 0.01f;
					}
					if (UI::PropertyGrid::Float("Max Distance", component.Properties.MaxDistance, 0.1f, 0.01f))
					{
						// Ensure MaxDistance is greater than MinDistance
						if (component.Properties.MaxDistance <= component.Properties.MinDistance)
							component.Properties.MaxDistance = component.Properties.MinDistance + 0.01f;
					}
				}

				UI::PropertyGrid::End();
			}
		}

	private:
		void RenderAudioClipSelector(AudioSourceComponent& component)
		{
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
				auto audioAsset = m_AssetManager.GetAsset<AudioClip>(component.AudioClipHandle);
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
				auto audioAsset = m_AssetManager.GetAssetByPath<AudioClip>(selectedAudioClip);
				if (audioAsset != nullptr)
					component.AudioClipHandle = audioAsset->GetUUID();
				else
				{
					auto audioAsset = m_AssetManager.Load<AudioClip>(selectedAudioClip);
					component.AudioClipHandle = audioAsset ? audioAsset->GetUUID() : (UUID)Constants::InvalidUUID;
				}
			}

			if (ImGui::BeginPopup("ChooseAudioClipPopup"))
			{
				if (ImGui::MenuItem("Load from file..."))
				{
					std::string fileTypes = DragDropUtils::DragDropPayloadTypeToExtension(DragDropPayloadType::AssetAudioClip);
					std::string defaultDir = (ProjectManager::GetActive()->GetAssetDirectory() / "Audio").string();
					std::string audioFile = FileDialog::OpenFile(defaultDir.c_str(), std::format("Audio ({})", fileTypes).c_str(), fileTypes.c_str());
					if (!audioFile.empty())
					{
						auto audioAsset = m_AssetManager.Load<AudioClip>(audioFile);
						if (audioAsset)
							component.AudioClipHandle = audioAsset->GetUUID();
					}
				}

				ImGui::EndPopup();
			}
			
		}
	};

}