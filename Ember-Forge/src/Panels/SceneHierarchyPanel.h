#pragma once

#include "Panel.h"

namespace Ember {

	class SceneHierarchyPanel : public Panel
	{
	public:
		SceneHierarchyPanel();
		virtual ~SceneHierarchyPanel();

		void OnEvent(Event& event) override;
		void OnImGuiRender() override;

	private:

	};
}