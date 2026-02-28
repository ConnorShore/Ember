#pragma once

#include "Ember/Core/Core.h"
#include "Ember/Core/Window.h"

#include <GLFW/glfw3.h>

namespace Ember {
	namespace Windows {

		class Window : public Ember::Window
		{
		public:
			Window(const WindowConfig& config);
			virtual ~Window();

			virtual void OnUpdate() override;

			virtual unsigned int GetWidth() override { return m_Width; }
			virtual unsigned int GetHeight() override { return m_Height; }

		private:
			unsigned int m_Width, m_Height;
			std::string m_Title;

			ScopedPtr<GLFWwindow> m_Window;
		};

	}
}