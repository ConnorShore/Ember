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
			Core = 1,
			Rendering = 2,
			Lighting = 3,
			Physics = 4,
			Animation = 5,
			Scripting = 6
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