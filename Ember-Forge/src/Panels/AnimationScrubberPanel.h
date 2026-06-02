#pragma once
#include "Panel.h"
#include "Ember/Animation/Animation.h"

namespace Ember {

	class AnimationScrubberPanel : public Panel
	{
	public:
		AnimationScrubberPanel(EditorContext* context);
		virtual ~AnimationScrubberPanel();

		void OnAttach() override;
		void OnImGuiRender() override;

		void SetCurrentAnimation(SharedPtr<Animation> animation);
		SharedPtr<Animation> GetCurrentAnimation() const { return m_CurrentAnimation; }

	private:
		void RenderEventListPane();
		void RenderScrubberPane();

	private:
		SharedPtr<Animation> m_CurrentAnimation = nullptr;

		// Scrubber state
		float m_CurrentTime = 0.0f;

		// Event selection and creation state
		int m_SelectedEventIndex = -1;
		char m_NewEventName[128] = "";
	};

}