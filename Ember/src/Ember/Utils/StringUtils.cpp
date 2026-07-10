#include "ebpch.h"
#include "StringUtils.h"

namespace Ember {

	std::string StringUtils::GetBaseName(const std::string& name)
	{
		std::string baseName = name;

		// Trim any existing "(N)" or " (N)" suffix to find the true base name
		if (!baseName.empty() && baseName.back() == ')')
		{
			size_t openParen = baseName.rfind('(');
			if (openParen != std::string::npos && openParen < baseName.length() - 1)
			{
				// Verify everything between the parentheses is a valid digit
				bool isNumber = true;
				for (size_t i = openParen + 1; i < baseName.length() - 1; ++i)
				{
					if (!std::isdigit(baseName[i]))
					{
						isNumber = false;
						break;
					}
				}

				if (isNumber)
				{
					// Strip the "(N)" suffix
					baseName = baseName.substr(0, openParen);

					// Strip any trailing spaces (so "Box (1)" becomes "Box" rather than "Box ")
					while (!baseName.empty() && baseName.back() == ' ')
					{
						baseName.pop_back();
					}
				}
			}
		}

		return baseName;
	}

}