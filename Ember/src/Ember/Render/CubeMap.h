#pragma once

#include "Texture.h"

namespace Ember {

	enum class CubeMapFace
	{
		None = 0,
		Right = 1,	// Positive X
		Left = 2,	// Negative X
		Top = 3,	// Positive Y
		Bottom = 4,	// Negative Y
		Back = 5,	// Positive Z
		Front = 6	// Negative Z
	};

	class CubeMap : public Texture
	{
	public:
		CubeMap(const std::string& name, const std::string& filePath)
			: Texture(name, filePath) {
		}
		CubeMap(UUID uuid, const std::string& name, const std::string& filePath)
			: Texture(uuid, name, filePath) {
		}

		virtual void SetData(CubeMapFace face, const void* data, uint32_t size) = 0;

		static SharedPtr<CubeMap> Create(uint32_t resolution);

		virtual bool operator==(const SharedPtr<CubeMap>& other) const = 0;
	private:

	};
}