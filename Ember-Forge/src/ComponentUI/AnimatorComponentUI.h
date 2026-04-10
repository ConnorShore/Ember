#pragma once


#include "ComponentUI.h"
#include "Ui/PropertyGrid.h"

#include <imgui/imgui.h>

namespace Ember {

	class AnimatorComponentUI : public ComponentUI<AnimatorComponent>
	{
	public:
		AnimatorComponentUI(EditorContext* context) : ComponentUI(context) { m_CanRemove = false; }
		inline const char* GetName() const override { return "Animator Component"; }

	protected:
		inline void RenderComponentImpl(AnimatorComponent& component) override
		{
			if (UI::PropertyGrid::Begin("AnimatorComponentProps"))
			{
				// Current Animation
				RenderAnimatorPicker(component);

				// Playback speed
				UI::PropertyGrid::Float("Playback Speed", component.PlaybackSpeed, 0.05f, -100.0f, 100.0f);

				// Looping
				UI::PropertyGrid::Checkbox("Loop", component.Loop);

				UI::PropertyGrid::End();

			}
		}

		inline void RenderAnimatorPicker(AnimatorComponent& component)
		{
			auto& assetManager = Application::Instance().GetAssetManager();

			std::string animName;
			if (component.CurrentAnimationHandle == Constants::InvalidUUID)
				animName = "None";
			else
				animName = assetManager.GetAsset<Animation>(component.CurrentAnimationHandle)->GetName();

			if (UI::PropertyGrid::BeginComboBox("Animation", animName.c_str()))
			{
				// Add an option for "None"
				if (UI::PropertyGrid::ComboBoxItem("None", component.CurrentAnimationHandle == Constants::InvalidUUID))
				{
					component.CurrentAnimationHandle = Constants::InvalidUUID;
					component.CurrentTime = 0.0f; // Reset animation time when changing animation
				}

				ImGui::Separator();

				// Show all animations in the asset manager
				auto animations = assetManager.GetAssetsOfType<Animation>();
				for (auto& anim : animations)
				{
					bool isSelected = component.CurrentAnimationHandle == anim->GetUUID();
					if (UI::PropertyGrid::ComboBoxItem(anim->GetName().c_str(), isSelected))
					{
						component.CurrentAnimationHandle = anim->GetUUID();
						component.CurrentTime = 0.0f; // Reset animation time when changing animation
					}
					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}

				UI::PropertyGrid::EndComboBox();
			}
		}
	};

}