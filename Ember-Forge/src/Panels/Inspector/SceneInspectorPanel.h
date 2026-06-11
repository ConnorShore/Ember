#pragma once

#include "InspectorPanel.h"

namespace Ember {

	class SceneInspectorPanel : public InspectorPanelContent
	{
	public:
		SceneInspectorPanel(EditorContext* context);
		virtual ~SceneInspectorPanel();

		virtual void OnImGuiRender() override;

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

	private:
		void DrawEntityHeader(Entity entity);
		void RenderEntityComponents(Entity entity);
		ComponentUIBase* FindComponentUI(ComponentType componentType, Entity entity) const;

	private:
		std::map<Category, std::vector<ScopedPtr<ComponentUIBase>>> m_ComponentUIs;
	};

}