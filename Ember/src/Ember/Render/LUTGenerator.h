#pragma once

#include "Ember/Core/Core.h"
#include "Texture2D.h"

#include <vector>
#include <string>

namespace Ember {

	class LUTGenerator
	{
	public:
		inline static SharedPtr<Texture2D> GenerateNeutralColorLUT(const std::string& name) {
			const int width = 256;
			const int height = 16;
			std::vector<unsigned char> data(width * height * 3);

			for (int y = 0; y < height; ++y)
			{
				for (int x = 0; x < width; ++x)
				{
					// The 256 width is divided into 16 blocks of 16 pixels.
					int blockX = x / 16; // Which blue block are we in? (0 to 15)
					int rCol = x % 16;   // Red changes horizontally within the block (0 to 15)
					int gRow = y;        // Green changes vertically (0 to 15)

					// Normalize to 0.0 -> 1.0, then scale to 0 -> 255
					float r = (float)rCol / 15.0f;
					float g = (float)gRow / 15.0f;
					float b = (float)blockX / 15.0f;

					int index = (y * width + x) * 3;
					data[index + 0] = static_cast<unsigned char>(r * 255.0f);
					data[index + 1] = static_cast<unsigned char>(g * 255.0f);
					data[index + 2] = static_cast<unsigned char>(b * 255.0f);
				}
			}

			return Texture2D::Create(name, width, height, data.data());
		}
	};


}