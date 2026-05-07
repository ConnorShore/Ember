#pragma once

#include "NavNode.h"

#include <vector>

namespace Ember {

	class Scene;

	class NavigationGrid
	{
	public:
		static std::vector<std::vector<NavNode>> Generate(const Vector3f& center, float gridSizeX, float gridSizeY, float nodeSpacing);
		static void RenderGeneratedGrid(const std::vector<std::vector<NavNode>>& grid, bool selected = false);
		static void RenderUngeneratedGrid(const Vector3f& center, float gridSizeX, float gridSizeY, bool selected = false);
	};

}