#pragma once

#include "Texture.h"

#include "Ember/Core/Core.h"
#include "Ember/Asset/Asset.h"

#include <string>
#include <unordered_map>

namespace Ember {

	//////////////////////////////////////////////////////////////////////////
	// Texture 2D
	//////////////////////////////////////////////////////////////////////////

	class Texture2DArray : public Texture
	{
	public:
		Texture2DArray(const std::string& name, const std::string& filePath)
			: Texture(name, filePath) {
		}
		Texture2DArray(UUID uuid, const std::string& name, const std::string& filePath)
			: Texture(uuid, name, filePath) {
		}

		virtual ~Texture2DArray() = default;

		virtual void SetData(const void* data, uint32_t index, uint32_t size) = 0;
		virtual TextureFormat GetFormat() const = 0;

		//static SharedPtr<Texture2D> Create();
		//static SharedPtr<Texture2D> Create(const std::string& filePath);
		//static SharedPtr<Texture2D> Create(const std::string& name, const std::string& filePath);
		//static SharedPtr<Texture2D> Create(UUID uuid, const std::string& name, const std::string& filePath);
		//static SharedPtr<Texture2D> Create(const std::string& name, uint32_t width, uint32_t height, const void* data);
		static SharedPtr<Texture2DArray> Create(UUID uuid, const std::string& name, TextureFormat format, uint32_t width, uint32_t height, uint32_t layers, const void* data, bool clampToEdge = false);
		//static SharedPtr<Texture2D> Create(UUID uuid, const std::string& name, uint32_t width, uint32_t height, const void* data);

		virtual bool operator==(const SharedPtr<Texture2DArray>& other) const = 0;
	};
}