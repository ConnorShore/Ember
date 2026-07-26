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
			// Only LDR RGBA8 textures are cooked. Every texture decoded from an image file is forced to
			// 4 channels (RGBA8 for LDR, RGBA16F for HDR) — but GetData()/the cooked-data upload path
			// both hardcode GL_UNSIGNED_BYTE, which silently corrupts float formats. Rather than cook a
			// broken RGBA16F blob (e.g. HDR skyboxes), skip it and let the runtime decode the source.
			if (!texture || texture->GetFormat() != TextureFormat::RGBA8)
				return false;

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

			// Only RGBA8 cooks are valid (see SaveCooked). Reject anything else — including stale
			// pre-fix RGBA16F cooks — so the caller falls back to decoding the authoring source.
			if (static_cast<TextureFormat>(format) != TextureFormat::RGBA8)
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
			// Resolve to the authoring source path (recovering it if the registry handed us a stale
			// ".bin"), then derive the cooked sibling from that. Every tier prefers one form but falls
			// back to the other so a missing/corrupt cook degrades to the source instead of a black
			// texture or a hard failure.
			const std::filesystem::path source = AssetSerializationMode::ResolveSourcePath(filePath);
			const std::filesystem::path cooked = AssetSerializationMode::GetCookedPath(source);

			auto tryCooked = [&]() -> SharedPtr<Texture2D> {
				if (auto tex = LoadCooked(uuid, name, cooked.string()))
				{
					// Keep the asset's identity on the source, never the .bin sidecar, so the registry
					// records a portable, decodable path.
					tex->SetFilePath(source.string());
					return tex;
				}
				return nullptr;
			};
			auto trySource = [&]() -> SharedPtr<Texture2D> {
				// stb_image cannot decode a raw ".bin"; only attempt a source decode on a real image.
				if (source.extension() == ".bin")
					return nullptr;
				return LoadSource(uuid, name, source.string());
			};

			switch (AssetSerializationMode::GetRuntimeLoadTier())
			{
			case RuntimeAssetLoadTier::ForceSourceYaml:
				if (auto tex = trySource()) return tex;
				return tryCooked();
			case RuntimeAssetLoadTier::ForceCookedBinary:
				if (auto tex = tryCooked()) return tex;
				return trySource();
			case RuntimeAssetLoadTier::Auto:
			default:
				// Reading raw pixels beats decoding a compressed image on the main thread, so prefer a
				// fresh cooked sibling; otherwise decode the source.
				if (IsCookedTextureFresh(source, cooked))
					if (auto tex = tryCooked()) return tex;
				if (auto tex = trySource()) return tex;
				return tryCooked();
			}
		}
	};

}