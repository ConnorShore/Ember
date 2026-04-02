#pragma once

#include "EditorContext.h"
#include "Ui/Nodes.h"

#include <Ember.h>
#include <string>

namespace Ember {

	//////////////////////////////////////////////////////////////////////////
	// Component UI Base Class
	//////////////////////////////////////////////////////////////////////////

	class ComponentUIBase
	{
	public:
		virtual ~ComponentUIBase() = default;
		virtual void Render(Entity entity) = 0;
		virtual const char* GetName() const = 0;
		virtual void CreateComponentForEntity(Entity entity) = 0;
	};

	//////////////////////////////////////////////////////////////////////////
	// Component UI Template Class
	//////////////////////////////////////////////////////////////////////////

	template<typename T>
	class ComponentUI : public ComponentUIBase
	{
	public:
		ComponentUI(EditorContext* context) : m_Context(context) {}
		virtual ~ComponentUI() = default;

		virtual void Render(Entity entity) override
		{
			RenderComponent(GetName(), entity, [this](T& component) { RenderComponentImpl(component); });
		}

		virtual const char* GetName() const = 0;
		virtual void CreateComponentForEntity(Entity entity) override
		{
			if (entity.ContainsComponent<T>())
			{
				EB_CORE_WARN("Entity already contains a component of type %s!", GetName());
				return;
			}

			T comp = {};
			m_Context->ActiveScene->AttachComponent(entity, comp);
		}

	protected:
		virtual void RenderComponentImpl(T& component) = 0;

	protected:
		EditorContext* m_Context;
		bool m_CanRemove = true;

    private:
		template<typename UIFunction>
		inline void RenderComponent(const char* name, Entity entity, UIFunction uiFunction)
		{
			// If the entity doesn't have this component, don't draw anything!
			if (!entity.ContainsComponent<T>())
				return;

			auto& component = entity.GetComponent<T>();
			if (UI::Nodes::BeginExpandableNode(std::string(name), true, [&]() {
				m_Context->PendingComponentRemovals[entity].push_back(entity.GetComponentType<T>());
				}))
			{
				uiFunction(component);
				UI::Nodes::EndExpandableNode();
			}
		}
	};

}