#pragma once

#include "Ember/Core/Core.h"
#include "Ember/Core/Window.h"

#include <GLFW/glfw3.h>

namespace Ember {
	namespace Windows {

		class Window : public Ember::Window
		{
		public:
			using EventCallbackFunc = std::function<void(Event&)>;

			Window(const WindowConfig& config);
			virtual ~Window();

			virtual void Clear() override;
			virtual void PollEvents() override;
			virtual void SwapBuffers() override;

			inline virtual unsigned int GetWidth() override { return m_WindowData.Width; }
			inline virtual unsigned int GetHeight() override { return m_WindowData.Height; }
			inline virtual void SetEventCallback(const EventCallbackFunc& callback) override { m_WindowData.EventCallback = callback; }
			inline virtual void* GetNativeWindow() const override { return m_Window; }

		private:
			GLFWwindow* m_Window;

			struct WindowData
			{
				std::string Title;
				unsigned int Width, Height;
				EventCallbackFunc EventCallback;
			} m_WindowData;
		};

	}
}