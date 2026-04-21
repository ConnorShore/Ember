#include "ebpch.h"
#include "AnimationSystem.h"

#include "Ember/Asset/Animation.h"

#include "Ember/Scene/Scene.h"

namespace Ember {

	// --- HELPER FUNCTIONS ---
	// Finds the keyframe index just BEFORE the current time
	template<typename T>
	static size_t GetKeyframeIndex(const std::vector<T>& keys, float animationTime)
	{
		for (size_t i = 0; i < keys.size() - 1; ++i) {
			if (animationTime < keys[i + 1].TimeStamp)
				return i;
		}
		return keys.size() > 0 ? keys.size() - 1 : 0;
	}

	// Calculates the blend factor (0.0 to 1.0) between two keyframes
	static float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime)
	{
		float midwayLength = animationTime - lastTimeStamp;
		float framesDiff = nextTimeStamp - lastTimeStamp;
		return midwayLength / framesDiff;
	}

	static Vector3f EvaluatePosition(const BoneAnimationTrack& track, float time)
	{
		if (track.PositionKeyframes.size() == 1)
			return track.PositionKeyframes[0].Position;

		size_t p0Index = GetKeyframeIndex(track.PositionKeyframes, time);
		size_t p1Index = (p0Index + 1) % track.PositionKeyframes.size(); // Safer wrap-around

		float factor = 0.0f;
		if (p0Index != p1Index)
			factor = GetScaleFactor(track.PositionKeyframes[p0Index].TimeStamp, track.PositionKeyframes[p1Index].TimeStamp, time);

		return Math::Mix(track.PositionKeyframes[p0Index].Position, track.PositionKeyframes[p1Index].Position, factor);
	}

	static Quaternion EvaluateRotation(const BoneAnimationTrack& track, float time)
	{
		if (track.RotationKeyframes.size() == 1)
			return track.RotationKeyframes[0].Rotation;

		size_t p0Index = GetKeyframeIndex(track.RotationKeyframes, time);
		size_t p1Index = (p0Index + 1) % track.RotationKeyframes.size();

		float factor = 0.0f;
		if (p0Index != p1Index)
			factor = GetScaleFactor(track.RotationKeyframes[p0Index].TimeStamp, track.RotationKeyframes[p1Index].TimeStamp, time);

		Quaternion rot = Math::Slerp(track.RotationKeyframes[p0Index].Rotation, track.RotationKeyframes[p1Index].Rotation, factor);
		return Math::Normalize(rot);
	}

	// Helper to find a track for a specific bone ID
	static const BoneAnimationTrack* GetTrack(const SharedPtr<Animation>& anim, uint32_t boneID)
	{
		if (!anim) 
			return nullptr;

		// TODO: Optimize in future with bone to track cache:
		// i.e. return animation->Tracks[BoneToTrackMap[boneID]]
		for (const auto& track : anim->GetTracks()) 
		{
			if (track.BoneID == boneID)
				return &track;
		}

		return nullptr;
	}

	// ------------------------

	void AnimationSystem::OnAttach()
	{
		EB_CORE_INFO("Animation System attached!");
	}

	void AnimationSystem::OnDetach()
	{
		EB_CORE_INFO("Animation System detached!");
	}

	void AnimationSystem::OnUpdate(TimeStep delta, Scene* scene)
	{
		auto& assetManager = Application::Instance().GetAssetManager();
		View view = scene->GetRegistry().ActiveQuery<AnimatorComponent>();

		for (EntityID entity : view)
		{
			auto& animator = scene->GetRegistry().GetComponent<AnimatorComponent>(entity);

			// Safety checks
			if (animator.SkeletonHandle == Constants::InvalidUUID || animator.CurrentAnimationHandle == Constants::InvalidUUID)
				continue;

			auto skeleton = assetManager.GetAsset<Skeleton>(animator.SkeletonHandle);
			auto animation = assetManager.GetAsset<Animation>(animator.CurrentAnimationHandle);
			auto prevAnimation = animator.PreviousAnimationHandle != Constants::InvalidUUID ? assetManager.GetAsset<Animation>(animator.PreviousAnimationHandle) : nullptr;

			const auto& bones = skeleton->GetBones();
			const auto& invBindTransforms = skeleton->GetInverseBindTransforms();

			float blendWeight = 1.0f;

			// Advance the clock
			if (animator.IsPlaying)
			{
				float duration = animation->GetDuration();

				//If a animation is currently crossfading, we need to handle the logic
				if (animator.PreviousAnimationHandle != Constants::InvalidUUID && prevAnimation)
				{
					animator.PreviousTime += (delta * animator.PlaybackSpeed);
					if (animator.Loop)
						animator.PreviousTime = fmod(animator.PreviousTime, prevAnimation->GetDuration());

					// Calculate Blend Weight (0.0 to 1.0)
					animator.CurrentBlendTime += delta;
					blendWeight = std::clamp(animator.CurrentBlendTime / animator.BlendDuration, 0.0f, 1.0f);

					if (blendWeight >= 1.0f)
					{
						// Blend finished, clear the previous state
						animator.PreviousAnimationHandle = Constants::InvalidUUID;
						animator.CurrentBlendTime = 0.0f;
						animator.BlendDuration = 0.0f;
					}
				}

				animator.CurrentTime += (delta * animator.PlaybackSpeed);  //  TODO: Add playback speed multiplier here later

				if (animator.PlaybackSpeed > 0.0f && animator.CurrentTime > duration)
				{
					if (animator.Loop)
						animator.CurrentTime = fmod(animator.CurrentTime, duration);
					else 
					{
						animator.CurrentTime = duration;	// Clamp to end
						animator.IsPlaying = false;
					}
				}
				else if (animator.PlaybackSpeed < 0.0f && animator.CurrentTime <= 0.0f)
				{
					animator.CurrentTime = std::fmod(animator.CurrentTime, duration);

					if (animator.CurrentTime < 0.0f)
						animator.CurrentTime += duration;
					else
					{
						animator.CurrentTime = 0.0f;	// Clamp to start
						animator.IsPlaying = false;
					}
				}
			}

			// Pre-allocate arrays for this frame's math
			std::vector<Matrix4f> localTransforms(bones.size());
			std::vector<Matrix4f> globalTransforms(bones.size());

			// 1. Interpolate and Blend!
			for (uint32_t i = 0; i < bones.size(); i++)
			{
				// Default to bind pose
				Vector3f currentPos = bones[i].LocalBindPoseTransform.Translation;
				Quaternion currentRot = bones[i].LocalBindPoseTransform.Rotation;

				// Evaluate Current Animation
				if (animation) {
					if (const auto* track = GetTrack(animation, i)) {
						if (track->PositionKeyframes.size() > 0)
							currentPos = EvaluatePosition(*track, animator.CurrentTime);
						if (track->RotationKeyframes.size() > 0)
							currentRot = EvaluateRotation(*track, animator.CurrentTime);
					}
				}

				// Evaluate Previous Animation & BLEND
				if (blendWeight < 1.0f && prevAnimation)
				{
					Vector3f prevPos = bones[i].LocalBindPoseTransform.Translation;
					Quaternion prevRot = bones[i].LocalBindPoseTransform.Rotation;

					if (const auto* prevTrack = GetTrack(prevAnimation, i)) {
						if (prevTrack->PositionKeyframes.size() > 0)
							prevPos = EvaluatePosition(*prevTrack, animator.PreviousTime);
						if (prevTrack->RotationKeyframes.size() > 0)
							prevRot = EvaluateRotation(*prevTrack, animator.PreviousTime);
					}

					// If blendWeight is 0.2, it takes 80% of prev and 20% of current.
					currentPos = Math::Mix(prevPos, currentPos, blendWeight);
					currentRot = glm::normalize(Math::Slerp(prevRot, currentRot, blendWeight));
				}

				// Combine into the new Local Matrix
				localTransforms[i] = Math::Translate(currentPos) * Math::ToMatrix4f(currentRot);
			}


			// Build global pose hierarchy
			// We can loop linearly because glTF guarantees parent nodes appear before children in the array
			for (size_t i = 0; i < bones.size(); i++)
			{
				if (bones[i].ParentID == -1) {
					globalTransforms[i] = localTransforms[i]; // Root bone
				}
				else {
					// Parent Global * Child Local
					globalTransforms[i] = globalTransforms[bones[i].ParentID] * localTransforms[i];
				}
			}

			// Apply inverse bind pose to get final bone matrices for skinning
			if (animator.BoneMatrices.size() < bones.size()) {
				animator.BoneMatrices.resize(bones.size(), Matrix4f(1.0f));
			}

			for (size_t i = 0; i < bones.size(); i++)
			{
				// FinalMatrix = GlobalPose * InverseBindPose
				animator.BoneMatrices[i] = globalTransforms[i] * invBindTransforms[i];
			}
		}
	}
}