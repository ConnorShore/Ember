#pragma once

#include "Ember/Core/Layer.h"
#include "Ember/Core/Time.h"

namespace Ember {

	class ImGuiLayer : public Layer 
	{
	public:
		ImGuiLayer();
		virtual ~ImGuiLayer();

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		void BeginFrame();
		void EndFrame();
	private:
		// TODO: Setup Key/Mouse events

	private:

	};
}