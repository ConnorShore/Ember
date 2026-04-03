#pragma once

#include <Ember/Core/SharedPointer.h>
#include <Ember/Core/Time.h>
#include <Ember/Event/Event.h>

#include "EditorContext.h"

#include <string>

namespace Ember {

	class Panel : public SharedResource
	{
	public:
		Panel(const std::string& title, EditorContext* context) : m_Title(title), m_Context(context) {}
		virtual ~Panel() = default;

		virtual void OnAttach() {}
		virtual void OnEvent(Event& event) {}
		virtual void OnUpdate(TimeStep delta) {}
		virtual void OnImGuiRender() = 0;

		inline void SetContext(EditorContext* context) { m_Context = context; }
		inline const std::string& GetTitle() const { return m_Title; }

	protected:
		std::string m_Title;
		EditorContext* m_Context;
	};
}