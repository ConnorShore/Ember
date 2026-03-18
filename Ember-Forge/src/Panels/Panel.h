#pragma once

#include <Ember.h>

#include <string>

namespace Ember {

	class Panel : public SharedResource
	{
	public:
		Panel(const std::string& title) : m_Title(title) {}
		virtual ~Panel() = default;

		virtual void OnEvent(Event& event) {};
		virtual void OnImGuiRender() = 0;

		inline void SetContext(const SharedPtr<Scene>& context) { m_Context = context; }
		inline const std::string& GetTitle() const { return m_Title; }

	protected:
		std::string m_Title;
		SharedPtr<Scene> m_Context;
	};
}