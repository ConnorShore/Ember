#pragma once

#include "Asset.h"
#include "Ember/Render/Texture2D.h"

#include <stb_truetype.h>

namespace Ember {

	class Font : public Asset
	{
	public:
		// Assume we only care about standard ASCII (Space to Tilde) for now
		static constexpr uint32_t FirstChar = 32;
		static constexpr uint32_t CharCount = 96;

		Font(const std::string& name, const std::string& filePath);
		Font(UUID uuid, const std::string& name, const std::string& filePath);

		virtual ~Font() = default;

		SharedPtr<Texture2D> GetAtlasTexture() const { return m_AtlasTexture; }

		// The engine needs this to know the width/UVs of a specific letter
		const stbtt_bakedchar* GetGlyphData() const { return m_GlyphData; }

		static AssetType GetStaticType() { return AssetType::Font; }

	private:
		SharedPtr<Texture2D> m_AtlasTexture;
		stbtt_bakedchar m_GlyphData[CharCount];
	};

}