#pragma once

#include "Core.h"
#include "Time.h"
#include "Ember/Event/Event.h"
#include "Ember/Render/Shader.h"

#include <string>

namespace Ember {

	class Layer
	{
	public:
		Layer(const std::string& name)
			: m_Name(name) { }
		virtual ~Layer() { }

		virtual void OnAttach() {}
		virtual void OnDetatch() {};
		virtual void OnEvent(const Event& event) {}
		virtual void OnUpdate(TimeStep delta) {};
		virtual void OnImGuiRender(TimeStep delta) {};

		inline const std::string& GetName() const { return m_Name; }

	protected:
		void RegisterShader(const std::string& filePath);
		SharedPtr<Shader> GetShader(const std::string& name);

	private:
		// TODO: Make m_Name debug only?
		std::string m_Name;
	};

}