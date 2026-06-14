#pragma once

#include "Ember/Core/Core.h"
#include "Ember/Animation/AnimationController.h"
#include "Ember/Asset/AssetSerializationMode.h"

namespace Ember {

	class AnimationControllerSerializer
	{
	public:
		// Editor/source format (.ebcontroller): preserves full graph metadata.
		static bool SerializeSource(const std::filesystem::path& filepath, const SharedPtr<AnimationController>& animationController);
		static SharedPtr<AnimationController> DeserializeSource(UUID uuid, const std::filesystem::path& filepath);

		// Cooked/runtime format (.bin): strips editor-only metadata.
		static bool SerializeCooked(const std::filesystem::path& filepath, const SharedPtr<AnimationController>& animationController);
		static SharedPtr<AnimationController> DeserializeCooked(UUID uuid, const std::filesystem::path& filepath);

		// Backwards-compatible wrappers used by existing code paths.
		static bool Serialize(const std::filesystem::path& filepath, const SharedPtr<AnimationController>& animationController);
		static SharedPtr<AnimationController> Deserialize(UUID uuid, const std::filesystem::path& filepath);

		static void SetRuntimeLoadTier(RuntimeAssetLoadTier tier);
		static RuntimeAssetLoadTier GetRuntimeLoadTier();
	};

}
