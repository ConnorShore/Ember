#pragma once

#include <string>
#include <functional>

#include "Ember/Event/Event.h"
#include "Ember/Core/Core.h"

namespace Ember {

	struct WindowConfig
	{
		unsigned int Width, Height;
		std::string Title;

		WindowConfig(const std::string& title = "Ember Engine",
			unsigned int width = 1280,
			unsigned int height = 720)
			: Title(title), Width(width), Height(height)
		{
		}
	};

	class Window
	{
	public:
		virtual ~Window() = default;

		virtual void Clear() = 0;	// TODO: This will move to rendering
		virtual void OnUpdate() = 0;

		virtual unsigned int GetWidth() = 0;
		virtual unsigned int GetHeight() = 0;

		virtual void SetEventCallback(const std::function<void(Event&)>& callback) = 0;

		virtual void* GetNativeWindow() const = 0;

		static ScopedPtr<Window> Create(const WindowConfig& config = WindowConfig());
	};

}