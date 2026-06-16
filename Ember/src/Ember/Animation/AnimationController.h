#pragma once

#include "AnimationParameter.h"
#include "AnimationLayer.h"
#include "AnimationStateMachine.h"

#include "Ember/Asset/Asset.h"

namespace Ember {

	class AnimationController : public Asset
	{
	public:
		AnimationController(UUID uuid, const std::string& name, const std::string& filePath)
			: Asset(uuid, name, filePath, AssetType::AnimationController) { }
		AnimationController(const std::string& name, const std::string& filePath)
			: AnimationController(UUID(), name, filePath) { }

		inline AnimationLayer& CreateLayer(const std::string& layerName)
		{
			AnimationLayer layer;
			layer.Name = layerName;
			m_Layers.push_back(layer);
			return m_Layers.back();
		}

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
