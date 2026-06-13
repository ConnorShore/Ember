#pragma once

#include "AnimationParameter.h"
#include "AnimationStateMachine.h"

#include "Ember/Asset/Asset.h"

namespace Ember {

	struct AnimationLayer
	{
		std::string Name = "Base Layer";
		float Weight = 1.0f;
		UUID MaskHandle = Constants::InvalidUUID; // Points to an .ebmask asset (Invalid = Full Body)
		AnimationStateMachine StateMachine;
	};

	class AnimationController : public Asset
	{
	public:
		AnimationController(UUID uuid, const std::string& name, const std::string& filePath)
			: Asset(uuid, name, filePath, AssetType::AnimationController) { }
		AnimationController(const std::string& name, const std::string& filePath)
			: AnimationController(UUID(), name, filePath) { }

		inline std::unordered_map<std::string, AnimationParameter>& GetParameters() { return m_Parameters; }
		inline const std::unordered_map<std::string, AnimationParameter>& GetParameters() const { return m_Parameters; }
		inline void SetParameters(const std::unordered_map<std::string, AnimationParameter>& parameters) { m_Parameters = parameters; }

		inline std::vector<AnimationLayer>& GetLayers() { return m_Layers; }
		inline const std::vector<AnimationLayer>& GetLayers() const { return m_Layers; }
		inline void SetLayers(const std::vector<AnimationLayer>& layers) { m_Layers = layers; }

		inline static AssetType GetStaticType() { return AssetType::AnimationController; }

	private:
		std::unordered_map<std::string, AnimationParameter> m_Parameters;
		std::vector<AnimationLayer> m_Layers;
	};

}
