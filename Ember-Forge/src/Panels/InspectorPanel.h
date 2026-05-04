#pragma once

#include "Panel.h"

#include "ComponentUI/ComponentUI.h"

#include <map>
#include <vector>
#include <string>

namespace Ember {

	class InspectorPanel : public Panel
	{
	public:
		enum class Category
		{
			None = 0,
			Core,
			Rendering,
			Lighting,
			Physics,
			Audio,
			Animation,
			Scripting
		};

	public:
		InspectorPanel(EditorContext* context);
		virtual ~InspectorPanel();

		void OnEvent(Event& event) override;
		void OnImGuiRender() override;

	private:
		void DrawEntityHeader(Entity entity);

	private:
		std::map<Category, std::vector<ScopedPtr<ComponentUIBase>>> m_ComponentUIs;
	};
}