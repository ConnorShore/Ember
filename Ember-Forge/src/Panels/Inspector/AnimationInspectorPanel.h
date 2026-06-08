#pragma once

#include "InspectorPanel.h"
#include "GraphNodeUI/GraphNodeUI.h"
#include <Ember/Animation/AnimationParameter.h>
#include <Ember/Animation/AnimationTransition.h>

namespace Ember {

	class AssetManager;

	class AnimationInspectorPanel : public InspectorPanelContent
	{
	public:
		AnimationInspectorPanel(EditorContext* context);
		virtual ~AnimationInspectorPanel();

		virtual void OnImGuiRender() override;

	private:
		// Core Rendering
		void RenderAnimationState(AnimationState* animState);
		void RenderAnimationTransition(AnimationTransition* animTransition);
		void RenderAnimationParameters();

		// Sub-Components
		void RenderTransitionConditions(AnimationTransition* animTransition);
		void RenderAnimationParametersTable();
		void RenderAddParameterPopup();

		// Utilities
		const char* ParameterTypeToString(AnimationParameterType type) const;

	private:
		AssetManager& m_AssetManager;

		std::string m_NewParameterName;
		AnimationParameterType m_NewParameterType = AnimationParameterType::Float;

		std::string m_RenamingParameterName;
		std::string m_RenameBuffer;
	};
}