#pragma once

#include <string>

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

		virtual void OnUpdate() = 0;

		virtual unsigned int GetWidth() = 0;
		virtual unsigned int GetHeight() = 0;

		static Window* Create(const WindowConfig& config = WindowConfig());
	};

}