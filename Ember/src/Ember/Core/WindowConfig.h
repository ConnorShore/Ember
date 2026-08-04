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
			const std::filesystem::path& iconPath = "Ember/assets/images/EmberIcon.png",
			bool startMaximized = false)
			: Title(title), Width(width), Height(height), IconPath(iconPath), StartMaximized(startMaximized)
		{
		}
	};

}