#pragma once

#include <filesystem>

namespace Ember {
	enum class RuntimeAssetLoadTier
	{
		Auto,
		ForceSourceYaml,
		ForceCookedBinary
	};

	class AssetSerializationMode
	{
	public:
		static void SetRuntimeLoadTier(RuntimeAssetLoadTier tier)
		{
			s_RuntimeLoadTier = tier;
		}

		static RuntimeAssetLoadTier GetRuntimeLoadTier()
		{
			return s_RuntimeLoadTier;
		}

		// The cooked binary sibling for an authoring source path (foo.png -> foo.bin).
		static std::filesystem::path GetCookedPath(const std::filesystem::path& path)
		{
			auto cooked = path;
			cooked.replace_extension(".bin");
			return cooked;
		}

		// The asset registry must always record the *source* authoring path so it stays portable and
		// so source-loading (e.g. stb_image) is never handed a raw ".bin" blob it can't decode. Older
		// registries were poisoned with cooked ".bin" paths because cooked loaders used to overwrite
		// the in-memory FilePath. Given such a stale ".bin" path, recover the real source sibling
		// (same stem, any non-.bin extension) that sits next to it on disk. Non-.bin paths, and ".bin"
		// paths with no source sibling, are returned unchanged.
		static std::filesystem::path ResolveSourcePath(const std::filesystem::path& path)
		{
			if (path.extension() != ".bin")
				return path;

			std::error_code ec;
			const auto dir = path.parent_path();
			const auto stem = path.stem();
			if (dir.empty())
				return path;

			// Use the error_code-based iteration (non-throwing constructor + increment) so a permission
			// or race error while scanning never escapes as an exception from an asset load.
			std::filesystem::directory_iterator it(dir, ec), end;
			for (; !ec && it != end; it.increment(ec))
			{
				const auto& candidate = it->path();
				if (candidate.extension() != ".bin" && candidate.stem() == stem)
					return candidate;
			}
			return path;
		}

	private:
		inline static RuntimeAssetLoadTier s_RuntimeLoadTier = RuntimeAssetLoadTier::Auto;
	};
}
