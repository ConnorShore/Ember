#include "ebpch.h"
#include "Font.h"

#include <stb_truetype.h>

namespace Ember {

	Font::Font(const std::string& name, const std::string& filePath)
		:Font(UUID(), name, filePath)
	{
	}

	Font::Font(UUID uuid, const std::string& name, const std::string& filePath)
		: Asset(uuid, name, filePath, AssetType::Font)
	{
		// Read the TTF file into a binary buffer
		std::ifstream file(filePath, std::ios::binary | std::ios::ate);
		if (!file.is_open())
		{
			EB_CORE_ERROR("Failed to load font file: {}", filePath);
			return;
		}

		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);
		std::vector<unsigned char> ttfBuffer(size);
		if (!file.read(reinterpret_cast<char*>(ttfBuffer.data()), size))
		{
			EB_CORE_ERROR("Failed to read font file data!");
			return;
		}

		// Prepare the memory for the generated texture (512x512 is standard for basic fonts)
		const int atlasWidth = 512;
		const int atlasHeight = 512;
		const float fontSize = 32.0f; // Bake at 32 pixels high

		std::vector<unsigned char> bitmapBuffer(atlasWidth * atlasHeight);

		// 3. Let stb_truetype do the heavy lifting!
		// This generates the bitmap array AND fills our m_GlyphData array with the UV coordinates
		int result = stbtt_BakeFontBitmap(
			ttfBuffer.data(), 0, fontSize,
			bitmapBuffer.data(), atlasWidth, atlasHeight,
			FirstChar, CharCount, m_GlyphData
		);

		if (result <= 0)
		{
			EB_CORE_ERROR("Failed to bake font bitmap! Try increasing atlas size.");
			return;
		}

		// 4. Create the Texture2D using your programmatic constructor
		// We use an R8 (1 byte per pixel) format because stbtt generated a grayscale image
		m_AtlasTexture = Texture2D::Create(
			UUID(),
			"FontAtlas",
			TextureFormat::R8,
			atlasWidth,
			atlasHeight,
			bitmapBuffer.data(),
			true // clampToEdge = true prevents text bleeding
		);
	}

}