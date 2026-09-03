#pragma once

#include "Ember/Asset/ParticleEffect.h"
#include "AssetSerializationMode.h"

#include "Ember/Core/Core.h"

#include <ryml.hpp>
#include <ryml_std.hpp>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace Ember {

	class AssetManager;

	class ParticleEffectSerializer
	{
	public:
		static bool SerializeSource(const std::filesystem::path& filepath, const SharedPtr<ParticleEffect>& effect);
		static SharedPtr<ParticleEffect> DeserializeSource(UUID uuid, const std::filesystem::path& filepath);

		static bool SerializeCooked(const std::filesystem::path& filepath, const SharedPtr<ParticleEffect>& effect);
		static SharedPtr<ParticleEffect> DeserializeCooked(UUID uuid, const std::filesystem::path& filepath);

		static bool Serialize(const std::filesystem::path& filepath, const SharedPtr<ParticleEffect>& effect);
		static SharedPtr<ParticleEffect> Deserialize(UUID uuid, const std::filesystem::path& filepath);

		static void SerializeProperties(ryml::NodeRef node, const ParticleEffectProperties& properties);
		static void DeserializeProperties(ryml::NodeRef node, ParticleEffectProperties& properties);
	};

}
