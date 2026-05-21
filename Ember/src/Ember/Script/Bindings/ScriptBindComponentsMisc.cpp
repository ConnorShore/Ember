#include "ebpch.h"
#include "ScriptBindComponents.h"
#include "Ember/ECS/Component/Components.h"

namespace Ember {
	void BindMiscComponents(sol::state& state)
	{
		state.new_usertype<AnimatorComponent>("AnimatorComponent",
			"CurrentAnimationHandle", &AnimatorComponent::CurrentAnimationHandle,
			"CurrentTime", sol::property(
				[](AnimatorComponent& c) { return (float)c.CurrentTime; },
				[](AnimatorComponent& c, float time) { c.CurrentTime = time; }
			),
			"PlaybackSpeed", &AnimatorComponent::PlaybackSpeed,
			"IsPlaying", &AnimatorComponent::IsPlaying,
			"Loop", &AnimatorComponent::Loop,
			"Crossfade", sol::overload(
				[](AnimatorComponent& c, const std::string& name, float blendDuration) { c.CrossfadeToAnimation(name, blendDuration); },
				[](AnimatorComponent& c, const std::string& name) { c.CrossfadeToAnimation(name, 0.0f); },
				[](AnimatorComponent& c, UUID targetAnim, float blendDuration) { 
					auto& assetManager = Application::Instance().GetAssetManager();
					auto animationAsset = assetManager.GetAsset<Animation>(targetAnim);
					c.CrossfadeToAnimation(animationAsset ? animationAsset->GetName() : "", blendDuration);
				}
			),
			"Play", sol::overload(
				[](AnimatorComponent& c, const std::string& name) { c.PlayAnimation(name);  },
				[](AnimatorComponent& c, const std::string& name, float playbackSpeed) { c.PlayAnimation(name, playbackSpeed); },
				[](AnimatorComponent& c, const std::string& name, float playbackSpeed, float blendDuration) { c.PlayAnimation(name, playbackSpeed, blendDuration); }
			),
			"PlayLoop", sol::overload(
				[](AnimatorComponent& c, const std::string& name) { c.PlayLoopAnimation(name);  },
				[](AnimatorComponent& c, const std::string& name, float playbackSpeed) { c.PlayLoopAnimation(name, playbackSpeed); },
				[](AnimatorComponent& c, const std::string& name, float playbackSpeed, float blendDuration) { c.PlayLoopAnimation(name, playbackSpeed, blendDuration); }
			)
		);

		state.new_usertype<LifetimeComponent>("LifetimeComponent",
			"Lifetime", &LifetimeComponent::Lifetime
		);
	}
}