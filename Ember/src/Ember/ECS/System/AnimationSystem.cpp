#include "ebpch.h"
#include "AnimationSystem.h"
#include "ScriptSystem.h"

#include "Ember/Animation/AnimationStateMachine.h"
#include "Ember/Animation/Animation.h"
#include "Ember/Animation/AnimationCondition.h"

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

	static Vector3f EvaluateScale(const BoneAnimationTrack& track, float time)
	{
		if (track.ScaleKeyframes.size() == 1)
			return track.ScaleKeyframes[0].Scale;

		size_t p0Index = GetKeyframeIndex(track.ScaleKeyframes, time);
		size_t p1Index = (p0Index + 1) % track.ScaleKeyframes.size();

		float factor = 0.0f;
		if (p0Index != p1Index)
			factor = GetScaleFactor(track.ScaleKeyframes[p0Index].TimeStamp, track.ScaleKeyframes[p1Index].TimeStamp, time);

		return Math::Mix(track.ScaleKeyframes[p0Index].Scale, track.ScaleKeyframes[p1Index].Scale, factor);
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

	static const AnimationState* ResolveState(const SharedPtr<AnimationStateMachine>& stateMachine, UUID stateId)
	{
		if (!stateMachine || stateId == Constants::InvalidUUID)
			return nullptr;

		const auto& states = stateMachine->GetStates();
		auto stateIt = states.find(stateId);
		return stateIt != states.end() ? &stateIt->second : nullptr;
	}

	static const AnimationState* ResolveCurrentState(const SharedPtr<AnimationStateMachine>& stateMachine, AnimatorComponent& animator)
	{
		if (!stateMachine)
			return nullptr;

		const auto& states = stateMachine->GetStates();
		if (states.empty())
		{
			animator.CurrentStateId = Constants::InvalidUUID;
			return nullptr;
		}

		if (animator.CurrentStateId != Constants::InvalidUUID)
		{
			auto currentStateIt = states.find(animator.CurrentStateId);
			if (currentStateIt != states.end())
				return &currentStateIt->second;
		}

		const UUID& defaultState = stateMachine->GetDefaultState();
		if (defaultState != Constants::InvalidUUID && states.contains(defaultState))
		{
			animator.CurrentStateId = defaultState;
			animator.CurrentTime = 0.0f;
			animator.PreviousStateId = Constants::InvalidUUID;
			animator.PreviousTime = 0.0f;
			animator.CurrentBlendTime = 0.0f;
			animator.ActiveBlendDuration = 0.0f;
			animator.IsBlending = false;
			return ResolveState(stateMachine, animator.CurrentStateId);
		}

		animator.CurrentStateId = Constants::InvalidUUID;
		return nullptr;
	}

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
			if (animator.SkeletonHandle == Constants::InvalidUUID)
				continue;

			auto skeleton = assetManager.GetAsset<Skeleton>(animator.SkeletonHandle);
			if (!skeleton)
				continue;

			auto animStateMachine = animator.AnimationStateMachineHandle != Constants::InvalidUUID ? assetManager.GetAsset<AnimationStateMachine>(animator.AnimationStateMachineHandle) : nullptr;
			const AnimationState* currentState = ResolveCurrentState(animStateMachine, animator);

			// See if we need to make a transition (check all transition conditions for the current state to see if any are met)
			if (animStateMachine && currentState && animStateMachine->GetTransitions().contains(animator.CurrentStateId))
			{
				const auto& transitions = animStateMachine->GetTransitions().at(animator.CurrentStateId);
				for (const auto& transition : transitions)
				{
					if (transition.ToStateId == animator.CurrentStateId || !ResolveState(animStateMachine, transition.ToStateId))
						continue;

					// Check condition to see if one is met, if so trigger transition and set prev and current states
					bool activateTransition = true;
					for (const auto& condition : transition.Conditions)
					{
						if (!AnimationConditionEvaluator::Evaluate(condition, animator.Blackboard.FloatParameters, animator.Blackboard.BoolParameters))
						{
							activateTransition = false;
							break;
						}
					}

					if (!activateTransition)
						continue;

					animator.PreviousStateId = animator.CurrentStateId;
					animator.PreviousTime = animator.CurrentTime;
					animator.CurrentStateId = transition.ToStateId;
					animator.CurrentTime = 0.0f;
					animator.CurrentBlendTime = 0.0f;
					animator.ActiveBlendDuration = std::max(transition.BlendDuration, 0.0f);
					animator.IsBlending = animator.ActiveBlendDuration > 0.0f;
					if (!animator.IsBlending)
					{
						animator.PreviousStateId = Constants::InvalidUUID;
						animator.PreviousTime = 0.0f;
					}
					currentState = ResolveState(animStateMachine, animator.CurrentStateId);
					break;
				}
			}

			const auto& bones = skeleton->GetBones();
			const auto& invBindTransforms = skeleton->GetInverseBindTransforms();

			float blendWeight = 1.0f;

			auto animation = currentState && currentState->AnimationHandle != Constants::InvalidUUID ? assetManager.GetAsset<Animation>(currentState->AnimationHandle) : nullptr;
			SharedPtr<Animation> prevAnimation = nullptr;
			if (currentState && animation)
			{
				float duration = animation->GetDuration();

				//If a animation is currently cross fading, we need to handle the logic
				if (animator.IsBlending)
				{
					const AnimationState* previousState = ResolveState(animStateMachine, animator.PreviousStateId);
					if (previousState && previousState->AnimationHandle != Constants::InvalidUUID)
						prevAnimation = assetManager.GetAsset<Animation>(previousState->AnimationHandle);

					if (!previousState || !prevAnimation || animator.ActiveBlendDuration <= 0.0f)
					{
						animator.PreviousStateId = Constants::InvalidUUID;
						animator.CurrentBlendTime = 0.0f;
						animator.ActiveBlendDuration = 0.0f;
						animator.IsBlending = false;
					}
					else
					{
						animator.PreviousTime += (delta * previousState->BasePlaybackSpeed);
						float previousDuration = prevAnimation->GetDuration();
						if (previousDuration > 0.0f)
						{
							if (previousState->Looping)
							{
								animator.PreviousTime = fmod(animator.PreviousTime, previousDuration);
								if (animator.PreviousTime < 0.0f)
									animator.PreviousTime += previousDuration;
							}
							else
							{
								animator.PreviousTime = std::clamp(animator.PreviousTime.Seconds(), 0.0f, previousDuration);
							}
						}

						// Calculate Blend Weight (0.0 to 1.0)
						animator.CurrentBlendTime += delta;
						blendWeight = std::clamp(animator.CurrentBlendTime / animator.ActiveBlendDuration, 0.0f, 1.0f);

						if (blendWeight >= 1.0f)
						{
							// Blend finished, clear the previous state
							animator.PreviousStateId = Constants::InvalidUUID;
							animator.CurrentBlendTime = 0.0f;
							animator.ActiveBlendDuration = 0.0f;
							animator.IsBlending = false;
						}
					}
				}

				float lastFrameTime = animator.CurrentTime;
				animator.CurrentTime += (delta * currentState->BasePlaybackSpeed);

				if (duration > 0.0f && currentState->BasePlaybackSpeed > 0.0f && animator.CurrentTime > duration)
				{
					if (currentState->Looping)
						animator.CurrentTime = fmod(animator.CurrentTime, duration);
					else
						animator.CurrentTime = duration;	// Clamp to end
				}
				else if (duration > 0.0f && currentState->BasePlaybackSpeed < 0.0f && animator.CurrentTime < 0.0f)
				{
					if (currentState->Looping)
					{
						animator.CurrentTime = std::fmod(animator.CurrentTime, duration);
						if (animator.CurrentTime < 0.0f)
							animator.CurrentTime += duration;
					}
					else
						animator.CurrentTime = 0.0f;	// Clamp to start
				}

				// See if any animation events need to be fired at this timestamp
				for (const auto& event : animation->GetEvents())
				{
					if (lastFrameTime < event.Timestamp && animator.CurrentTime >= event.Timestamp)
					{
						ScriptSystem::FireAnimationEvent(entity, event.Name, scene);
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
				Vector3f currentScale = Vector3f(1.0f);

				// Evaluate Current Animation
				if (animation) {
					if (const auto* track = GetTrack(animation, i)) {
						if (track->PositionKeyframes.size() > 0)
							currentPos = EvaluatePosition(*track, animator.CurrentTime);
						if (track->RotationKeyframes.size() > 0)
							currentRot = EvaluateRotation(*track, animator.CurrentTime);
						if (track->ScaleKeyframes.size() > 0)
							currentScale = EvaluateScale(*track, animator.CurrentTime);
					}
				}

				// Evaluate Previous Animation & BLEND
				if (blendWeight < 1.0f && prevAnimation)
				{
					Vector3f prevPos = bones[i].LocalBindPoseTransform.Translation;
					Quaternion prevRot = bones[i].LocalBindPoseTransform.Rotation;
					Vector3f prevScale = Vector3f(1.0f);

					if (const auto* prevTrack = GetTrack(prevAnimation, i)) {
						if (prevTrack->PositionKeyframes.size() > 0)
							prevPos = EvaluatePosition(*prevTrack, animator.PreviousTime);
						if (prevTrack->RotationKeyframes.size() > 0)
							prevRot = EvaluateRotation(*prevTrack, animator.PreviousTime);
						if (prevTrack->ScaleKeyframes.size() > 0)
							prevScale = EvaluateScale(*prevTrack, animator.PreviousTime);
					}

					// If blendWeight is 0.2, it takes 80% of prev and 20% of current.
					currentPos = Math::Mix(prevPos, currentPos, blendWeight);
					currentRot = glm::normalize(Math::Slerp(prevRot, currentRot, blendWeight));
					currentScale = Math::Mix(prevScale, currentScale, blendWeight);
				}

				// Combine into the new Local Matrix
				localTransforms[i] = Math::Translate(currentPos) * Math::ToMatrix4f(currentRot) * Math::Scale(currentScale);
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
			if (animator.BonePoseMatrices.size() < bones.size()) {
				animator.BonePoseMatrices.resize(bones.size(), Matrix4f(1.0f));
			}

			if (animator.BoneMatrices.size() < bones.size()) {
				animator.BoneMatrices.resize(bones.size(), Matrix4f(1.0f));
			}

			for (size_t i = 0; i < bones.size(); i++)
			{
				// FinalMatrix = GlobalPose * InverseBindPose
				animator.BonePoseMatrices[i] = globalTransforms[i];
				animator.BoneMatrices[i] = globalTransforms[i] * invBindTransforms[i];
			}
		}
	}

	static void ApplyPoseToAnimator(AnimatorComponent& animator, const SharedPtr<Skeleton>& skeleton, const SharedPtr<Animation>& animation, float timestamp)
	{
		const auto& bones = skeleton->GetBones();
		const auto& invBindTransforms = skeleton->GetInverseBindTransforms();

		// Pre-allocate arrays
		std::vector<Matrix4f> localTransforms(bones.size());
		std::vector<Matrix4f> globalTransforms(bones.size());

		if (!animation)
		{
			// Invalid Handle: Reset time and fill with default Bind Pose
			animator.CurrentTime = 0.0f;

			for (size_t i = 0; i < bones.size(); i++)
			{
				localTransforms[i] = Math::Translate(bones[i].LocalBindPoseTransform.Translation)
					* Math::ToMatrix4f(bones[i].LocalBindPoseTransform.Rotation)
					* Math::Scale(Vector3f(1.0f));
			}
		}
		else
		{
			// Valid Handle: Evaluate the Animation Curve
			animator.CurrentTime = timestamp;

			for (uint32_t i = 0; i < bones.size(); i++)
			{
				Vector3f currentPos = bones[i].LocalBindPoseTransform.Translation;
				Quaternion currentRot = bones[i].LocalBindPoseTransform.Rotation;
				Vector3f currentScale = Vector3f(1.0f);

				if (const auto* track = GetTrack(animation, i))
				{
					if (track->PositionKeyframes.size() > 0)
						currentPos = EvaluatePosition(*track, timestamp);
					if (track->RotationKeyframes.size() > 0)
						currentRot = EvaluateRotation(*track, timestamp);
					if (track->ScaleKeyframes.size() > 0)
						currentScale = EvaluateScale(*track, timestamp);
				}

				localTransforms[i] = Math::Translate(currentPos) * Math::ToMatrix4f(currentRot) * Math::Scale(currentScale);
			}
		}

		// --- 2. BUILD GLOBAL POSE HIERARCHY ---
		// Because we isolated the local transforms above, this math works perfectly for both cases!
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

		// --- 3. APPLY INVERSE BIND POSE ---
		if (animator.BonePoseMatrices.size() < bones.size()) {
			animator.BonePoseMatrices.resize(bones.size(), Matrix4f(1.0f));
		}

		if (animator.BoneMatrices.size() < bones.size()) {
			animator.BoneMatrices.resize(bones.size(), Matrix4f(1.0f));
		}

		for (size_t i = 0; i < bones.size(); i++)
		{
			// Final Matrix sent to the shader
			animator.BonePoseMatrices[i] = globalTransforms[i];
			animator.BoneMatrices[i] = globalTransforms[i] * invBindTransforms[i];
		}
	}

	void AnimationSystem::SetAnimationToTimestamp(Scene* scene, UUID animationHandle, Entity entity, float timestamp)
	{
		if (!entity.ContainsComponent<AnimatorComponent>())
			return;

		auto& animator = entity.GetComponent<AnimatorComponent>();
		auto& assetManager = Application::Instance().GetAssetManager();

		if (animator.SkeletonHandle == Constants::InvalidUUID)
			return;

		auto skeleton = assetManager.GetAsset<Skeleton>(animator.SkeletonHandle);
		if (!skeleton)
			return;

		auto animation = animationHandle != Constants::InvalidUUID ? assetManager.GetAsset<Animation>(animationHandle) : nullptr;
		ApplyPoseToAnimator(animator, skeleton, animation, timestamp);
	}

	void AnimationSystem::SetStateToTimestamp(Scene* scene, UUID currentStateId, Entity entity, float timestamp)
	{
		if (!entity.ContainsComponent<AnimatorComponent>())
			return;

		auto& animator = entity.GetComponent<AnimatorComponent>();
		auto& assetManager = Application::Instance().GetAssetManager();

		if (animator.SkeletonHandle == Constants::InvalidUUID)
			return;

		auto skeleton = assetManager.GetAsset<Skeleton>(animator.SkeletonHandle);
		if (!skeleton)
			return;

		auto stateMachine = animator.AnimationStateMachineHandle != Constants::InvalidUUID ? assetManager.GetAsset<AnimationStateMachine>(animator.AnimationStateMachineHandle) : nullptr;
		UUID resolvedStateId = currentStateId;
		if (stateMachine && resolvedStateId == Constants::InvalidUUID)
			resolvedStateId = stateMachine->GetDefaultState();

		const AnimationState* currentState = ResolveState(stateMachine, resolvedStateId);
		SharedPtr<Animation> animation = nullptr;
		if (currentState)
		{
			animator.CurrentStateId = resolvedStateId;
			if (currentState->AnimationHandle != Constants::InvalidUUID)
				animation = assetManager.GetAsset<Animation>(currentState->AnimationHandle);
		}

		ApplyPoseToAnimator(animator, skeleton, animation, timestamp);
	}

}