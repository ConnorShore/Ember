#include "ebpch.h"
#include "AnimationSystem.h"
#include "ScriptSystem.h"

#include "Ember/Animation/AnimationController.h"
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

	static const AnimationState* ResolveState(const AnimationStateMachine* stateMachine, UUID stateId)
	{
		if (!stateMachine || stateId == Constants::InvalidUUID)
			return nullptr;

		const auto& states = stateMachine->GetStates();
		auto stateIt = states.find(stateId);
		return stateIt != states.end() ? &stateIt->second : nullptr;
	}

	static const AnimationState* ResolveCurrentState(const AnimationStateMachine* stateMachine, AnimationLayerRuntime& runtime)
	{
		if (!stateMachine)
			return nullptr;

		const auto& states = stateMachine->GetStates();
		if (states.empty())
		{
			runtime.CurrentStateId = Constants::InvalidUUID;
			return nullptr;
		}

		if (runtime.CurrentStateId != Constants::InvalidUUID)
		{
			auto currentStateIt = states.find(runtime.CurrentStateId);
			if (currentStateIt != states.end())
				return &currentStateIt->second;
		}

		const UUID& defaultState = stateMachine->GetDefaultState();
		if (defaultState != Constants::InvalidUUID && states.contains(defaultState))
		{
			runtime.CurrentStateId = defaultState;
			runtime.CurrentTime = 0.0f;
			runtime.PreviousStateId = Constants::InvalidUUID;
			runtime.PreviousTime = 0.0f;
			runtime.CurrentBlendTime = 0.0f;
			runtime.ActiveBlendDuration = 0.0f;
			runtime.IsBlending = false;
			return ResolveState(stateMachine, runtime.CurrentStateId);
		}

		runtime.CurrentStateId = Constants::InvalidUUID;
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
			if (animator.LayerStates.empty())
			{
				animator.LayerStates.emplace_back();
			}

			// Safety checks
			if (animator.SkeletonHandle == Constants::InvalidUUID)
				continue;

			auto skeleton = assetManager.GetAsset<Skeleton>(animator.SkeletonHandle);
			if (!skeleton)
				continue;

			// TODO: Loop over each layer and apply the animation state machine logic for each layer
			//	Need to account for layer blending and masking as well
			auto controller = animator.ControllerHandle != Constants::InvalidUUID ? assetManager.GetAsset<AnimationController>(animator.ControllerHandle) : nullptr;
			if (controller->GetLayers().empty())
				continue;

			size_t numLayers = controller->GetLayers().size();
			for (size_t i = 0; i < numLayers; i++)
			{
				// TODO: Should I add a layer state or break if num layers is bigger than layer states
				if (i >= animator.LayerStates.size())
				{
					EB_CORE_WARN("Animator layer states is empty for layer {}. Placing blank layer state.", i);
					animator.LayerStates.emplace_back();
				}

				auto& runtime = animator.LayerStates[i];

				AnimationLayer& layer = controller->GetLayer(i);
				AnimationStateMachine* animStateMachine = &layer.StateMachine;
				const AnimationState* currentState = ResolveCurrentState(animStateMachine, runtime);

				std::unordered_map<std::string, AnimationParameter> effectiveParameters;
				if (controller)
					effectiveParameters = controller->GetParameters();

				// Log warnings if animator blackboard contains parameters not in the animation state machine's parameter list
				// Edit: Removed this for now as I'm not sure its needed and it was spamming the log a lot.
				// TODO: Make it so it logs this only once per parameter per animator instead of every frame
				for (auto parameter : animator.Blackboard.Parameters)
				{
					//if (controller && !controller->GetParameters().contains(parameter.first))
					//{
					//	EB_CORE_WARN("Animator has parameter '{}' that is not defined in the Animation State Machine!", parameter.first);
					//}

					effectiveParameters[parameter.first] = parameter.second;
				}

				// See if we need to make a transition (check all transition conditions for the current state to see if any are met)
				if (animStateMachine && currentState && animStateMachine->GetTransitions().contains(runtime.CurrentStateId))
				{
					const auto& transitions = animStateMachine->GetTransitions().at(runtime.CurrentStateId);
					for (const auto& transition : transitions)
					{
						if (transition.ToStateId == runtime.CurrentStateId || !ResolveState(animStateMachine, transition.ToStateId))
							continue;

						// Check condition to see if one is met, if so trigger transition and set prev and current states
						bool activateTransition = true;
						for (const auto& condition : transition.Conditions)
						{
							if (!AnimationConditionEvaluator::Evaluate(condition, effectiveParameters))
							{
								activateTransition = false;
								break;
							}
						}

						if (!activateTransition)
							continue;

						EB_CORE_INFO("Layer: {}; Transitioning from state '{}' to state '{}'!", controller->GetLayers()[0].Name, currentState->Name, ResolveState(animStateMachine, transition.ToStateId)->Name);

						runtime.PreviousStateId = runtime.CurrentStateId;
						runtime.PreviousTime = runtime.CurrentTime;
						runtime.CurrentStateId = transition.ToStateId;
						runtime.CurrentTime = 0.0f;
						runtime.CurrentBlendTime = 0.0f;
						runtime.ActiveBlendDuration = std::max(transition.BlendDuration, 0.0f);
						runtime.IsBlending = runtime.ActiveBlendDuration > 0.0f;
						if (!runtime.IsBlending)
						{
							runtime.PreviousStateId = Constants::InvalidUUID;
							runtime.PreviousTime = 0.0f;
						}
						currentState = ResolveState(animStateMachine, runtime.CurrentStateId);
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
					if (runtime.IsBlending)
					{
						const AnimationState* previousState = ResolveState(animStateMachine, runtime.PreviousStateId);
						if (previousState && previousState->AnimationHandle != Constants::InvalidUUID)
							prevAnimation = assetManager.GetAsset<Animation>(previousState->AnimationHandle);

						if (!previousState || !prevAnimation || runtime.ActiveBlendDuration <= 0.0f)
						{
							runtime.PreviousStateId = Constants::InvalidUUID;
							runtime.CurrentBlendTime = 0.0f;
							runtime.ActiveBlendDuration = 0.0f;
							runtime.IsBlending = false;
						}
						else
						{
							runtime.PreviousTime += (delta * previousState->BasePlaybackSpeed);
							float previousDuration = prevAnimation->GetDuration();
							if (previousDuration > 0.0f)
							{
								if (previousState->Looping)
								{
									runtime.PreviousTime = fmod(runtime.PreviousTime, previousDuration);
									if (runtime.PreviousTime < 0.0f)
										runtime.PreviousTime += previousDuration;
								}
								else
								{
									runtime.PreviousTime = std::clamp(runtime.PreviousTime.Seconds(), 0.0f, previousDuration);
								}
							}

							// Calculate Blend Weight (0.0 to 1.0)
							runtime.CurrentBlendTime += delta;
							blendWeight = std::clamp(runtime.CurrentBlendTime / runtime.ActiveBlendDuration, 0.0f, 1.0f);

							if (blendWeight >= 1.0f)
							{
								// Blend finished, clear the previous state
								runtime.PreviousStateId = Constants::InvalidUUID;
								runtime.CurrentBlendTime = 0.0f;
								runtime.ActiveBlendDuration = 0.0f;
								runtime.IsBlending = false;
							}
						}
					}

					float lastFrameTime = runtime.CurrentTime;
					runtime.CurrentTime += (delta * currentState->BasePlaybackSpeed);

					if (duration > 0.0f && currentState->BasePlaybackSpeed > 0.0f && runtime.CurrentTime > duration)
					{
						if (currentState->Looping)
							runtime.CurrentTime = fmod(runtime.CurrentTime, duration);
						else
							runtime.CurrentTime = duration;	// Clamp to end
					}
					else if (duration > 0.0f && currentState->BasePlaybackSpeed < 0.0f && runtime.CurrentTime < 0.0f)
					{
						if (currentState->Looping)
						{
							runtime.CurrentTime = std::fmod(runtime.CurrentTime, duration);
							if (runtime.CurrentTime < 0.0f)
								runtime.CurrentTime += duration;
						}
						else
							runtime.CurrentTime = 0.0f;	// Clamp to start
					}

					// See if any animation events need to be fired at this timestamp
					for (const auto& event : animation->GetEvents())
					{
						if (lastFrameTime < event.Timestamp && runtime.CurrentTime >= event.Timestamp)
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
								currentPos = EvaluatePosition(*track, runtime.CurrentTime);
							if (track->RotationKeyframes.size() > 0)
								currentRot = EvaluateRotation(*track, runtime.CurrentTime);
							if (track->ScaleKeyframes.size() > 0)
								currentScale = EvaluateScale(*track, runtime.CurrentTime);
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
								prevPos = EvaluatePosition(*prevTrack, runtime.PreviousTime);
							if (prevTrack->RotationKeyframes.size() > 0)
								prevRot = EvaluateRotation(*prevTrack, runtime.PreviousTime);
							if (prevTrack->ScaleKeyframes.size() > 0)
								prevScale = EvaluateScale(*prevTrack, runtime.PreviousTime);
						}

						// If blendWeight is 0.2, it takes 80% of prev and 20% of current.
						currentPos = Math::Mix(prevPos, currentPos, blendWeight);
						currentRot = glm::normalize(Math::Slerp(prevRot, currentRot, blendWeight));
						currentScale = Math::Mix(prevScale, currentScale, blendWeight);
					}

					// Combine into the new Local Matrix
					localTransforms[i] = Math::Translate(currentPos) * Math::ToMatrix4f(currentRot) * Math::Scale(currentScale);
				}

				// Set mask for bones based on mask for layer
				std::vector<float> boneWeights(bones.size(), 1.0f);
				if (layer.MaskHandle != Constants::InvalidUUID)
				{
					auto mask = assetManager.GetAsset<SkeletonMask>(layer.MaskHandle);
					if (mask)
					{
						for (size_t i = 0; i < bones.size(); i++)
						{
							boneWeights[i] = mask->GetBoneWeight(bones[i]);
						}
					}
				}

				// If no valid animation is playing in this layer, skip all bone writes.
				// This allows a state with no animation assigned to act as a full passthrough —
				// the previous layers' pose is preserved for any masked bones.
				if (!animation)
					continue;

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

				if (layer.Mode == AnimationLayerMode::Additive)
				{
					// Additive blending must operate in local TRS space.
					// Linear interpolation of raw matrices is invalid for transforms that contain
					// rotation: the columns become non-orthonormal after lerp, which causes the
					// translation delta to bleed into wrong axes (e.g. slide moves up instead of back).
					//
					// Correct approach:
					//   delta_pos = animLocalPos - bindLocalPos          (vector addition)
					//   delta_rot = animLocalRot * inverse(bindLocalRot) (quaternion multiplication)
					// These are applied to the accumulated local TRS from previous layers, which is
					// extracted by multiplying out the parent's inverse global.

					if (animator.BonePoseMatrices.size() < bones.size())
						animator.BonePoseMatrices.resize(bones.size(), Matrix4f(1.0f));
					if (animator.BoneMatrices.size() < bones.size())
						animator.BoneMatrices.resize(bones.size(), Matrix4f(1.0f));

					std::vector<Matrix4f> newGlobalTransforms(bones.size());

					for (size_t i = 0; i < bones.size(); i++)
					{
						const float effectiveWeight = boneWeights[i] * layer.Weight;

						// Animation local TRS (already cross-fade-blended into localTransforms[i]).
						// Extract position from the 4th column (column-major); rotation via quat_cast of upper-left 3x3.
						const Vector3f animLocalPos = Vector3f(localTransforms[i][3]);
						const Quaternion animLocalRot = glm::normalize(glm::quat_cast(Matrix3f(localTransforms[i])));

						// Bind pose local TRS
						const Vector3f bindLocalPos = bones[i].LocalBindPoseTransform.Translation;
						const Quaternion bindLocalRot = glm::normalize(bones[i].LocalBindPoseTransform.Rotation);

						// Delta from bind pose in local space
						const Vector3f deltaPos = animLocalPos - bindLocalPos;
						const Quaternion deltaRot = glm::normalize(animLocalRot * glm::inverse(bindLocalRot));

						// Accumulated local TRS from previous layers (recover local by removing parent's global)
						const Matrix4f accGlobal = animator.BonePoseMatrices[i];
						const Matrix4f accLocal = (bones[i].ParentID == -1)
							? accGlobal
							: Math::Inverse(animator.BonePoseMatrices[bones[i].ParentID]) * accGlobal;

						const Vector3f accLocalPos = Vector3f(accLocal[3]);
						const Quaternion accLocalRot = glm::normalize(glm::quat_cast(Matrix3f(accLocal)));

						// Apply weighted delta to accumulated local TRS
						const Vector3f newLocalPos = accLocalPos + effectiveWeight * deltaPos;
						const Quaternion newLocalRot = glm::normalize(
							accLocalRot * Math::Slerp(Quaternion(1.0f, 0.0f, 0.0f, 0.0f), deltaRot, effectiveWeight));

						const Matrix4f newLocal = Math::Translate(newLocalPos) * Math::ToMatrix4f(newLocalRot);

						// Rebuild global hierarchy
						newGlobalTransforms[i] = (bones[i].ParentID == -1)
							? newLocal
							: newGlobalTransforms[bones[i].ParentID] * newLocal;
					}

					for (size_t i = 0; i < bones.size(); i++)
					{
						animator.BonePoseMatrices[i] = newGlobalTransforms[i];
						animator.BoneMatrices[i] = newGlobalTransforms[i] * invBindTransforms[i];
					}
				}
				else
				{
					// Override: blend between the accumulated pose and this layer's pose.
					if (animator.BonePoseMatrices.size() < bones.size())
						animator.BonePoseMatrices.resize(bones.size(), Matrix4f(1.0f));
					if (animator.BoneMatrices.size() < bones.size())
						animator.BoneMatrices.resize(bones.size(), Matrix4f(1.0f));

					for (size_t i = 0; i < bones.size(); i++)
					{
						const float effectiveWeight = boneWeights[i] * layer.Weight;
						animator.BonePoseMatrices[i] = effectiveWeight * globalTransforms[i] + (1.0f - effectiveWeight) * animator.BonePoseMatrices[i];
						animator.BoneMatrices[i] = effectiveWeight * (globalTransforms[i] * invBindTransforms[i]) + (1.0f - effectiveWeight) * animator.BoneMatrices[i];
					}
				}
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
			if (animator.LayerStates.empty())
				animator.LayerStates.emplace_back();
			animator.LayerStates[0].CurrentTime = 0.0f;

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
			if (animator.LayerStates.empty())
				animator.LayerStates.emplace_back();
			animator.LayerStates[0].CurrentTime = timestamp;

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

		if (animator.LayerStates.empty())
			animator.LayerStates.emplace_back();
		auto& runtime = animator.LayerStates[0];

		auto controller = animator.ControllerHandle != Constants::InvalidUUID ? assetManager.GetAsset<AnimationController>(animator.ControllerHandle) : nullptr;
		AnimationStateMachine* stateMachine = nullptr;
		if (controller && !controller->GetLayers().empty())
			stateMachine = &controller->GetLayers()[0].StateMachine;
		UUID resolvedStateId = currentStateId;
		if (stateMachine && resolvedStateId == Constants::InvalidUUID)
			resolvedStateId = stateMachine->GetDefaultState();

		const AnimationState* currentState = ResolveState(stateMachine, resolvedStateId);
		SharedPtr<Animation> animation = nullptr;
		if (currentState)
		{
			runtime.CurrentStateId = resolvedStateId;
			if (currentState->AnimationHandle != Constants::InvalidUUID)
				animation = assetManager.GetAsset<Animation>(currentState->AnimationHandle);
		}

		ApplyPoseToAnimator(animator, skeleton, animation, timestamp);
	}

}