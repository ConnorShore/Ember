#pragma once

#include "Core.h"
#include "Time.h"

#include <string>

namespace Ember {

	class Layer
	{
	public:
		Layer(const std::string& name)
			: m_Name(name) { }
		virtual ~Layer() { }

		virtual void OnAttach() = 0;
		virtual void OnDetatch() = 0;

		virtual void OnUpdate(TimeStep delta) = 0;

		inline const std::string& GetName() const { return m_Name; }

	private:
		// TODO: Make m_Name debug only?
		std::string m_Name;
	};

}