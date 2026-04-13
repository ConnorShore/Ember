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
			Core = 0,
			Rendering = 1,
			Lighting = 2,
			Physics = 3,
			Animation = 4,
			Scripting = 5
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