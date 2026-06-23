#include "efpch.h"
#include "AnimationScrubberPanel.h"

#include <Ember/Core/Application.h>
#include <Ember/Animation/AnimationEvent.h>
#include <Ember/ECS/System/AnimationSystem.h>

#include <imgui/imgui.h>

#include <format>

namespace Ember {

	AnimationScrubberPanel::AnimationScrubberPanel(EditorContext* context)
		: Panel("Animation Timeline", context)
	{
	}

	AnimationScrubberPanel::~AnimationScrubberPanel()
	{
	}

	void AnimationScrubberPanel::OnAttach()
	{
	}

	void AnimationScrubberPanel::SetCurrentAnimation(SharedPtr<Animation> animation)
	{
		m_CurrentAnimation = animation;
		m_CurrentTime = 0.0f;
		m_SelectedEventIndex = -1;
		memset(m_NewEventName, 0, sizeof(m_NewEventName));
	}

	void AnimationScrubberPanel::OnImGuiRender()
	{
		ImGui::Begin(m_Title.c_str());

		// --- 1. ANIMATION SELECTION DROPDOWN ---
		auto animations = Application::Instance().GetAssetManager().GetAssetsOfType<Animation>();
		std::string previewName = m_CurrentAnimation ? m_CurrentAnimation->GetName() : "Select Animation...";

		if (ImGui::BeginCombo("Current Animation", previewName.c_str()))
		{
			// Default none option
			if (ImGui::Selectable("None", !m_CurrentAnimation))
			{
				SetCurrentAnimation(nullptr);
				m_CurrentTime = 0.0f;

				// Reset animation to bind pose
				auto animSystem = Application::Instance().GetSystem<AnimationSystem>();
				animSystem->SetAnimationToTimestamp(m_Context->ActiveScene().Ptr(), Constants::InvalidUUID, m_Context->SelectedEntity, m_CurrentTime);
			}
			ImGui::Separator();

			// Loop over all events
			for (const auto& anim : animations)
			{
				bool isSelected = (m_CurrentAnimation && m_CurrentAnimation->GetUUID() == anim->GetUUID());

				if (ImGui::Selectable(anim->GetName().c_str(), isSelected))
				{
					SetCurrentAnimation(anim);

					// Apply the animation at time 0 to show the initial keyframe pose
					if (m_Context->SelectedEntity != Constants::Entities::InvalidEntityID)
					{
						auto animSystem = Application::Instance().GetSystem<AnimationSystem>();
						animSystem->SetAnimationToTimestamp(m_Context->ActiveScene().Ptr(), anim->GetUUID(), m_Context->SelectedEntity, 0.0f);
					}
				}

				// Set the initial focus when opening the combo
				if (isSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		ImGui::Separator();
		ImGui::Spacing();

		if (!m_CurrentAnimation)
		{
			ImGui::TextDisabled("No animation selected.");
			ImGui::End();
			return;
		}

		// --- 2. DYNAMIC PANE SIZING ---
		float rightPaneWidth = 250.0f;
		// Calculate the left pane width by taking total available space and subtracting the right pane + spacing
		float leftPaneWidth = ImGui::GetContentRegionAvail().x - rightPaneWidth - ImGui::GetStyle().ItemSpacing.x;

		// --- 3. LEFT PANE: Scrubber and Timeline ---
		ImGui::BeginChild("ScrubberPane", ImVec2(leftPaneWidth, 0), true);
		RenderScrubberPane();
		ImGui::EndChild();

		ImGui::SameLine();

		// --- 4. RIGHT PANE: Event List ---
		ImGui::BeginChild("EventListPane", ImVec2(0, 0), true); // 0 width fills the remaining space
		RenderEventListPane();
		ImGui::EndChild();

		ImGui::End();
	}

	void AnimationScrubberPanel::RenderEventListPane()
	{
		ImGui::Text("Animation Events");
		ImGui::Separator();

		auto& events = m_CurrentAnimation->GetEvents();

		// Scrollable area for the list
		ImGui::BeginChild("EventScrollingRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - ImGui::GetStyle().ItemSpacing.y), false);
		for (int i = 0; i < events.size(); i++)
		{
			// Format the label nicely (e.g., "[1.25s] Footstep")
			std::string label = std::format("[{:.2f}s] {}", events[i].Timestamp, events[i].Name);

			if (ImGui::Selectable(label.c_str(), m_SelectedEventIndex == i))
			{
				m_SelectedEventIndex = i;

				// Quality of Life: Snap the scrubber to the event when clicked!
				m_CurrentTime = events[i].Timestamp;
			}
		}
		ImGui::EndChild();

		// Delete Button at the bottom
		ImGui::Separator();
		ImGui::BeginDisabled(m_SelectedEventIndex < 0 || m_SelectedEventIndex >= events.size());
		if (ImGui::Button("Delete Selected Event", ImVec2(-1, 0)))
		{
			m_CurrentAnimation->RemoveEvent(m_SelectedEventIndex);
			m_SelectedEventIndex = -1; // Reset selection after delete
		}
		ImGui::EndDisabled();
	}

	void AnimationScrubberPanel::RenderScrubberPane()
	{
		float animDuration = m_CurrentAnimation->GetDuration();
		Entity selectedEntity = m_Context->SelectedEntity;

		ImGui::Text("Timeline");
		ImGui::Separator();
		ImGui::Spacing();

		// --- THE SCRUBBER ---
		ImGui::SetNextItemWidth(-1);

		if (ImGui::SliderFloat("##Scrubber", &m_CurrentTime, 0.0f, animDuration, "%.3f s"))
		{
			// If the slider is moved, and we have a valid entity selected, set the pose
			if (selectedEntity != Constants::Entities::InvalidEntityID)
			{
				auto animSystem = Application::Instance().GetSystem<AnimationSystem>();
				animSystem->SetAnimationToTimestamp(m_Context->ActiveScene().Ptr(), m_CurrentAnimation->GetUUID(), selectedEntity, m_CurrentTime);
			}
		}

		// --- THE MARKERS ---
		ImVec2 rectMin = ImGui::GetItemRectMin(); // Top-Left of the slider bar
		ImVec2 rectMax = ImGui::GetItemRectMax(); // Bottom-Right of the slider bar
		ImVec2 sliderSize = ImVec2(rectMax.x - rectMin.x, rectMax.y - rectMin.y);
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		ImU32 markerColor = IM_COL32(255, 255, 0, 255); // Yellow ticks
		auto& events = m_CurrentAnimation->GetEvents();

		for (const auto& evt : events)
		{
			float timeRatio = evt.Timestamp / animDuration;
			float markerScreenX = rectMin.x + (timeRatio * sliderSize.x);

			// Draw a vertical line right over the slider bar
			ImVec2 lineTop(markerScreenX, rectMin.y);
			ImVec2 lineBottom(markerScreenX, rectMax.y);
			drawList->AddLine(lineTop, lineBottom, markerColor, 2.0f);
		}

		// (Optional) Here is where you would tell your active scene/model to update its pose based on m_CurrentTime!
		// e.g., m_Context->ActiveScene->GetSystem<AnimationSystem>()->SetPreviewTime(m_CurrentAnimation, m_CurrentTime);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// --- ADD NEW EVENT CONTROLS ---
		ImGui::Text("Add Event at %.3fs", m_CurrentTime);

		ImGui::SetNextItemWidth(200.0f); // Fixed width for the text input
		ImGui::InputTextWithHint("##EventNameInput", "Enter event name...", m_NewEventName, sizeof(m_NewEventName));

		ImGui::SameLine();

		ImGui::BeginDisabled(strlen(m_NewEventName) == 0); // Disable if text is empty
		if (ImGui::Button("Add Event"))
		{
			m_CurrentAnimation->AddEvent(std::string(m_NewEventName), m_CurrentTime);

			// Sort the events so they appear chronologically in the right pane
			std::sort(events.begin(), events.end(), [](const AnimationEvent& a, const AnimationEvent& b) {
				return a.Timestamp < b.Timestamp;
			});

			// Clear the input box for the next event
			memset(m_NewEventName, 0, sizeof(m_NewEventName));
		}
		ImGui::EndDisabled();
	}

}