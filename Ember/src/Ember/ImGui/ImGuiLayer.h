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

		// Keyboard/gamepad UI navigation is turned off while the game owns input, so arrow keys and
		// face buttons drive the game instead of walking the editor's widgets.
		void SetNavigationEnabled(bool enabled);
		bool IsNavigationEnabled() const { return m_NavigationEnabled; }

	private:
		// The nav flags requested at attach time, restored whenever navigation is re-enabled.
		int m_NavigationFlags = 0;
		bool m_NavigationEnabled = true;
	};
}