#pragma once

#include "GraphNodeUI/GraphNodeUI.h"

#include <Ember/Animation/AnimationStateMachine.h>
#include <imgui/imgui.h>

namespace Ember {

	class AnimationStateNodeUI : public GraphNodeUI
	{
	public:
		AnimationStateNodeUI(UUID stateId)
			: m_StateId(stateId) { }
		virtual ~AnimationStateNodeUI() = default;

		inline std::string GetName() const override
		{
			return "State: " + m_AnimationStateMachine->GetStates().at(m_StateId).Name;
		}

	protected:
		inline virtual void RenderNodeData() override
		{
			ImGui::Text("State ID: %llu", (uint64_t)m_StateId);
		}

	private:
		// TODO: May only need a handle on the AnimationState itself
		SharedPtr<AnimationStateMachine> m_AnimationStateMachine;
		UUID m_StateId;
	};

}