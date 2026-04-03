#pragma once

#include "Ember/Core/Core.h"
#include "Ember/Core/Window.h"
#include "Ember/Render/GraphicsContext.h"

#include <GLFW/glfw3.h>

namespace Ember {
	namespace Windows {

		class Window : public Ember::Window
		{
		public:
			using EventCallbackFunc = std::function<void(Event&)>;

			Window(const WindowConfig& config);
			virtual ~Window();

			virtual void OnUpdate() override;

			virtual void SetVSync(bool enabled) override;
			virtual bool IsVSync() const override;

			inline virtual uint32_t GetWidth() const override { return m_WindowData.Width; }
			inline virtual uint32_t GetHeight() const override { return m_WindowData.Height; }
			inline virtual void SetEventCallback(const EventCallbackFunc& callback) override { m_WindowData.EventCallback = callback; }
			inline virtual void* GetNativeWindow() const override { return m_Window; }

		private:
			void RegisterCallbacks();

		private:
			GLFWwindow* m_Window;
			ScopedPtr<GraphicsContext> m_GraphicsContext;

			struct WindowData
			{
				std::string Title;
				uint32_t Width, Height;
				EventCallbackFunc EventCallback;
				bool VSync;
			} m_WindowData;
		};

	}
}