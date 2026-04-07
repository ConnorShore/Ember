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
		View view = scene->GetRegistry().Query<AnimatorComponent>();

		for (EntityID entity : view)
		{
			auto& animator = scene->GetRegistry().GetComponent<AnimatorComponent>(entity);

			// Safety checks
			if (animator.SkeletonHandle == Constants::InvalidUUID || animator.CurrentAnimationHandle == Constants::InvalidUUID)
				continue;

			auto skeleton = assetManager.GetAsset<Skeleton>(animator.SkeletonHandle);
			auto animation = assetManager.GetAsset<Animation>(animator.CurrentAnimationHandle);

			const auto& bones = skeleton->GetBones();
			const auto& invBindTransforms = skeleton->GetInverseBindTransforms();

			// Advance the clock
			if (animator.IsPlaying)
			{
				animator.CurrentTime += delta;  //  TODO: Add playback speed multiplier here later

				if (animator.CurrentTime > animation->GetDuration()) {
					if (animator.Loop) {
						animator.CurrentTime = fmod(animator.CurrentTime, animation->GetDuration());
					}
					else {
						animator.CurrentTime = animation->GetDuration();
						animator.IsPlaying = false;
					}
				}
			}

			// Pre-allocate arrays for this frame's math
			std::vector<Matrix4f> localTransforms(bones.size());
			std::vector<Matrix4f> globalTransforms(bones.size());

			// Initialize all bones to their default bind pose 
			// (Important because some bones might not have an animation track!)
			for (size_t i = 0; i < bones.size(); i++) {
				localTransforms[i] = bones[i].LocalBindPoseTransform.ToMatrix();
			}

			// Interpolate the tracks
			for (const auto& track : animation->GetTracks())
			{
				uint32_t boneID = track.BoneID;
				Vector3f position = bones[boneID].LocalBindPoseTransform.Translation;
				Quaternion rotation = bones[boneID].LocalBindPoseTransform.Rotation;

				// Interpolate Position
				if (track.PositionKeyframes.size() == 1) {
					position = track.PositionKeyframes[0].Position;
				}
				else if (track.PositionKeyframes.size() > 1) {
					size_t p0Index = GetKeyframeIndex(track.PositionKeyframes, animator.CurrentTime);
					size_t p1Index = p0Index + 1;
					float factor = GetScaleFactor(track.PositionKeyframes[p0Index].TimeStamp, track.PositionKeyframes[p1Index].TimeStamp, animator.CurrentTime);
					position = Math::Mix(track.PositionKeyframes[p0Index].Position, track.PositionKeyframes[p1Index].Position, factor);
				}

				// Interpolate Rotation (SLERP)
				if (track.RotationKeyframes.size() == 1) {
					rotation = track.RotationKeyframes[0].Rotation;
				}
				else if (track.RotationKeyframes.size() > 1) {
					size_t p0Index = GetKeyframeIndex(track.RotationKeyframes, animator.CurrentTime);
					size_t p1Index = p0Index + 1;
					float factor = GetScaleFactor(track.RotationKeyframes[p0Index].TimeStamp, track.RotationKeyframes[p1Index].TimeStamp, animator.CurrentTime);
					rotation = Math::Slerp(track.RotationKeyframes[p0Index].Rotation, track.RotationKeyframes[p1Index].Rotation, factor);
					rotation = glm::normalize(rotation); // Always normalize after slerp to prevent scaling artifacts
				}

				// TODO: Add scale later

				// Combine into the new Local Matrix
				localTransforms[boneID] = Math::Translate(position) * Math::ToMatrix4f(rotation);
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