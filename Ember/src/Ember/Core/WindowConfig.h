#pragma once

#include <cstdint>
#include <string>
#include <filesystem>

namespace Ember {

	struct WindowConfig
	{
		uint32_t Width, Height;
		std::string Title;
		std::filesystem::path IconPath;
		bool StartMaximized;

		WindowConfig(const std::string& title = "Ember Engine",
			uint32_t width = 1280,
			uint32_t height = 720,
			// Left empty so this header stays free of filesystem work; Window::Create falls back to the
			// engine's default icon when nothing is set.
			const std::filesystem::path& iconPath = {},
			bool startMaximized = false)
			: Title(title), Width(width), Height(height), IconPath(iconPath), StartMaximized(startMaximized)
		{
		}
	};

}