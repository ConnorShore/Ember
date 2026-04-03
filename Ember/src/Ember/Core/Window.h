#pragma once

#include <string>
#include <functional>

#include "Ember/Event/Event.h"
#include "Ember/Core/Core.h"

namespace Ember {

	struct WindowConfig
	{
		uint32_t Width, Height;
		std::string Title;

		WindowConfig(const std::string& title = "Ember Engine",
			uint32_t width = 1280,
			uint32_t height = 720)
			: Title(title), Width(width), Height(height)
		{
		}
	};

	class Window
	{
	public:
		static constexpr uint32_t MaxWidth = 8192;
		static constexpr uint32_t MaxHeight = 8192;

	public:
		virtual ~Window() = default;

		virtual void OnUpdate() = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		virtual void SetVSync(bool enabled) = 0;
		virtual bool IsVSync() const = 0;

		virtual void SetEventCallback(const std::function<void(Event&)>& callback) = 0;

		virtual void* GetNativeWindow() const = 0;

		static ScopedPtr<Window> Create(const WindowConfig& config = WindowConfig());
	};

}