#pragma once

#include <string>

namespace Ember {

	class FileDialog
	{
	public:
		static std::string OpenFile(const char* initialDirectory = "", const char* filterName = "All Files (*.*)", const char* filterExt = "*.*");
		static std::string OpenDirectory();
		static std::string SaveFile(const char* initialDirectory = "", const char* initialFileName = "", const char* filterName = "All Files (*.*)", const char* filterExt = "*.*");
	};

}