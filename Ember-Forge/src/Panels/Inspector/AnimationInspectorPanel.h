#pragma once

#include "InspectorPanel.h"
#include "GraphNodeUI/GraphNodeUI.h"

#include <Ember/Animation/AnimationParameter.h>

namespace Ember {

	class AssetManager;

	class AnimationInspectorPanel : public InspectorPanelContent
	{
	public:
		AnimationInspectorPanel(EditorContext* context);
		virtual ~AnimationInspectorPanel();

		virtual void OnImGuiRender() override;

	private:
		void RenderAnimationState(AnimationState* animState);
		void RenderAnimationTransition(AnimationTransition* animTransition);
		void RenderAnimationParameters();
		const char* ParameterTypeToString(AnimationParameterType type) const;

	private:
		AssetManager& m_AssetManager;
		std::string m_NewParameterName;
		AnimationParameterType m_NewParameterType = AnimationParameterType::Float;
		std::string m_RenamingParameterName;
		std::string m_RenameBuffer;
	};

}