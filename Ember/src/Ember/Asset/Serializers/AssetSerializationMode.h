#pragma once

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

	private:
		inline static RuntimeAssetLoadTier s_RuntimeLoadTier = RuntimeAssetLoadTier::Auto;
	};
}
