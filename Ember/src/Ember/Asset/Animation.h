#pragma once

#include "Asset.h"

#include "Ember/Core/Time.h"
#include "Ember/Math/Math.h"

#include <vector>
#include <string>

namespace Ember {

	struct PositionKeyframe
	{
		TimeStep TimeStamp;
		Vector3f Position;
	};

	struct RotationKeyframe
	{
		TimeStep TimeStamp;
		Quaternion Rotation;
	};

	// TODO: Scale keyframes

	struct BoneAnimationTrack
	{
		uint32_t BoneID;
		std::vector<PositionKeyframe> PositionKeyframes;
		std::vector<RotationKeyframe> RotationKeyframes;
	};

	class Animation : public Asset
	{
	public:
		Animation(UUID uuid, const std::string& name, float duration, const std::vector<BoneAnimationTrack>& tracks)
			: Asset(uuid, name, "", AssetType::Animation), m_Duration(duration), m_Tracks(tracks) {
		}
		Animation(const std::string& name, float duration, const std::vector<BoneAnimationTrack>& tracks)
			: Animation(UUID(), name, duration, tracks) {
		}

		inline float GetDuration() const { return m_Duration; }
		inline const std::vector<BoneAnimationTrack>& GetTracks() const { return m_Tracks; }

		static AssetType GetStaticType() { return AssetType::Animation; }

	private:
		float m_Duration;
		std::vector<BoneAnimationTrack> m_Tracks;
	};
}