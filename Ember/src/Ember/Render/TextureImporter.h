#pragma once

#include "Texture2D.h"

#include "Ember/Core/Core.h"
#include "Ember/Asset/Serializers/AssetSerializationMode.h"

#include "stb_image_write.h"

#include <fstream>
#include <vector>
#include <filesystem>
#include <cstdlib>

namespace Ember {

	class TextureImporter
	{
	public:
		inline static bool SaveSource(const SharedPtr<Texture2D>& texture, const std::string& filePath)
		{
			stbi_flip_vertically_on_write(true);
			return stbi_write_png(filePath.c_str(), texture->GetWidth(), texture->GetHeight(),
				TextureFormatBytesPerPixel(texture->GetFormat()), texture->GetData(), 256 * 3);
		}

		inline static SharedPtr<Texture2D> LoadSource(const std::string& filePath)
		{
			return Texture2D::Create(filePath);
		}
		inline static SharedPtr<Texture2D> LoadSource(const std::string& name, const std::string& filePath)
		{
			return Texture2D::Create(name, filePath);
		}
		inline static SharedPtr<Texture2D> LoadSource(UUID uuid, const std::string& name, const std::string& filePath)
		{
			return Texture2D::Create(uuid, name, filePath);
		}

		inline static bool SaveCooked(const SharedPtr<Texture2D>& texture, const std::string& filePath)
		{
			auto cookedPath = std::filesystem::path(filePath);
			cookedPath.replace_extension(".bin");

			std::ofstream stream(cookedPath, std::ios::binary | std::ios::trunc);
			if (!stream.is_open())
				return false;

			constexpr uint32_t magic = 0x58425445; // EBTX
			constexpr uint32_t version = 1;
			uint32_t width = texture->GetWidth();
			uint32_t height = texture->GetHeight();
			uint32_t format = static_cast<uint32_t>(texture->GetFormat());
			uint32_t bytesPerPixel = static_cast<uint32_t>(TextureFormatBytesPerPixel(texture->GetFormat()));
			uint64_t dataSize = static_cast<uint64_t>(width) * static_cast<uint64_t>(height) * bytesPerPixel;

			stream.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
			stream.write(reinterpret_cast<const char*>(&version), sizeof(version));
			stream.write(reinterpret_cast<const char*>(&width), sizeof(width));
			stream.write(reinterpret_cast<const char*>(&height), sizeof(height));
			stream.write(reinterpret_cast<const char*>(&format), sizeof(format));
			stream.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));

			const void* data = texture->GetData();
			if (dataSize > 0)
				stream.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(dataSize));

			free(const_cast<void*>(data));
			return stream.good();
		}

		inline static SharedPtr<Texture2D> LoadCooked(UUID uuid, const std::string& name, const std::string& filePath)
		{
			std::ifstream stream(filePath, std::ios::binary);
			if (!stream.is_open())
				return nullptr;

			uint32_t magic = 0;
			uint32_t version = 0;
			uint32_t width = 0;
			uint32_t height = 0;
			uint32_t format = 0;
			uint64_t dataSize = 0;

			stream.read(reinterpret_cast<char*>(&magic), sizeof(magic));
			stream.read(reinterpret_cast<char*>(&version), sizeof(version));
			stream.read(reinterpret_cast<char*>(&width), sizeof(width));
			stream.read(reinterpret_cast<char*>(&height), sizeof(height));
			stream.read(reinterpret_cast<char*>(&format), sizeof(format));
			stream.read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize));

			constexpr uint32_t expectedMagic = 0x58425445; // EBTX
			if (magic != expectedMagic || version != 1)
				return nullptr;

			std::vector<uint8_t> pixels(static_cast<size_t>(dataSize));
			if (dataSize > 0)
				stream.read(reinterpret_cast<char*>(pixels.data()), static_cast<std::streamsize>(dataSize));

			if (!stream.good())
				return nullptr;

			auto texture = Texture2D::Create(uuid, name, static_cast<TextureFormat>(format), width, height, pixels.data());
			texture->SetFilePath(filePath);
			return texture;
		}

		inline static bool Save(const SharedPtr<Texture2D>& texture, const std::string& filePath)
		{
			return SaveSource(texture, filePath);
		}

		inline static SharedPtr<Texture2D> Load(const std::string& filePath)
		{
			return Load(std::filesystem::path(filePath).stem().string(), filePath);
		}

		inline static SharedPtr<Texture2D> Load(const std::string& name, const std::string& filePath)
		{
			return Load(UUID(), name, filePath);
		}

		// A cooked texture is safe to use if it exists and is at least as new as its source image, so
		// re-importing/editing the source (which rewrites the .png/.jpg) invalidates a stale cook.
		inline static bool IsCookedTextureFresh(const std::filesystem::path& source, const std::filesystem::path& cooked)
		{
			std::error_code ec;
			if (!std::filesystem::exists(cooked, ec) || ec)
				return false;

			auto cookedTime = std::filesystem::last_write_time(cooked, ec);
			if (ec)
				return false;

			auto sourceTime = std::filesystem::last_write_time(source, ec);
			if (ec)
				return true; // Source unreadable but the cook exists — prefer the cook over failing.

			return cookedTime >= sourceTime;
		}

		inline static SharedPtr<Texture2D> Load(UUID uuid, const std::string& name, const std::string& filePath)
		{
			switch (AssetSerializationMode::GetRuntimeLoadTier())
			{
			case RuntimeAssetLoadTier::ForceSourceYaml:
				return LoadSource(uuid, name, filePath);
			case RuntimeAssetLoadTier::ForceCookedBinary:
			{
				auto cookedPath = std::filesystem::path(filePath);
				cookedPath.replace_extension(".bin");
				return LoadCooked(uuid, name, cookedPath.string());
			}
			case RuntimeAssetLoadTier::Auto:
			default:
			{
				// The registered path already points at cooked binary data.
				if (std::filesystem::path(filePath).extension() == ".bin")
					return LoadCooked(uuid, name, filePath);

				// Source image (e.g. .png/.jpg): prefer a fresh cooked sibling when one exists — reading
				// raw pixels is far cheaper than decoding a compressed image on the main thread.
				auto cookedPath = std::filesystem::path(filePath);
				cookedPath.replace_extension(".bin");
				if (IsCookedTextureFresh(filePath, cookedPath))
				{
					if (auto cooked = LoadCooked(uuid, name, cookedPath.string()))
						return cooked;
					// Corrupt/unreadable cook — fall through to decoding the source instead of failing.
				}

				return LoadSource(uuid, name, filePath);
			}
			}
		}
	};

}