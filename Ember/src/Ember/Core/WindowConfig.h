#pragma once

#include <cstdint>
#include <string>

namespace Ember {

	struct WindowConfig
	{
		uint32_t Width, Height;
		std::string Title;
		bool StartMaximized;

		WindowConfig(const std::string& title = "Ember Engine",
			uint32_t width = 1280,
			uint32_t height = 720,
			bool startMaximized = false)
			: Title(title), Width(width), Height(height), StartMaximized(startMaximized)
		{
		}
	};

}