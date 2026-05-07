#pragma once

#include "Asset.h"
#include "AnimationEvent.h"

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

		inline void AddEvent(const std::string& eventName, float timestamp)
		{
			m_Events.push_back({ eventName, timestamp });
		}
		inline void RemoveEvent(uint32_t eventIndex)
		{
			EB_CORE_ASSERT(eventIndex < m_Events.size(), "Event index out of bounds!");
			m_Events.erase(m_Events.begin() + eventIndex);
		}
		inline void SetEvents(const std::vector<AnimationEvent>& events) { m_Events = events; }
		inline std::vector<AnimationEvent>& GetEvents() { return m_Events; }
		inline const std::vector<AnimationEvent>& GetEvents() const { return m_Events; }

		static AssetType GetStaticType() { return AssetType::Animation; }

	private:
		float m_Duration;
		std::vector<BoneAnimationTrack> m_Tracks;
		std::vector<AnimationEvent> m_Events;
	};
}