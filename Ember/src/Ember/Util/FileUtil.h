#pragma once

#include <string>

namespace Ember {

	class FileUtil
	{
	public:


		inline static std::string ExtractFileName(const std::string& filePath)
		{
			// Get filename with extension after slashes
			size_t lastSlash = filePath.find_last_of("/\\");
			lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;

			// Remove last dot
			size_t lastDot = filePath.find_last_of(".");
			lastDot = lastDot == std::string::npos ? filePath.length() : lastDot;
			return filePath.substr(lastSlash, lastDot - lastSlash);
		}
	};
}