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
			Transform = 0,
			Rendering,
			Lighting,
			Camera,
			Physics,
			Audio,
			Animation,
			Scripting,
			AI,
			UI,
			Gameplay,
			Effects,
			Miscellaneous
		};

	public:
		InspectorPanel(EditorContext* context);
		virtual ~InspectorPanel();

		void OnEvent(Event& event) override;
		void OnImGuiRender() override;

	private:
		void DrawEntityHeader(Entity entity);
		void RenderEntityComponents(Entity entity);
		ComponentUIBase* FindComponentUI(ComponentType componentType, Entity entity) const;

	private:
		std::map<Category, std::vector<ScopedPtr<ComponentUIBase>>> m_ComponentUIs;
	};
}