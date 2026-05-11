#pragma once

#include <cstdint>
#include <string>

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

}