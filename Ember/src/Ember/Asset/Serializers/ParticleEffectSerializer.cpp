#include "ebpch.h"
#include "ParticleEffectSerializer.h"
#include "Ember/Utils/SerializationUtils.h"

namespace {
	template<typename T>
	void WriteRaw(std::ofstream& stream, const T& value)
	{
		stream.write(reinterpret_cast<const char*>(&value), sizeof(T));
	}

	template<typename T>
	bool ReadRaw(std::ifstream& stream, T& value)
	{
		stream.read(reinterpret_cast<char*>(&value), sizeof(T));
		return stream.good();
	}
}

namespace Ember {
	namespace {
		constexpr uint32_t PARTICLE_EFFECT_SOURCE_FILE_VERSION = 2; // 2 added BlendMode
		constexpr uint32_t PARTICLE_EFFECT_COOKED_MAGIC = 0x45425045; // EBPE
		constexpr uint32_t PARTICLE_EFFECT_COOKED_VERSION = 2; // 2 added BlendMode

		std::filesystem::path GetCookedPath(const std::filesystem::path& filepath)
		{
			auto cookedPath = filepath;
			cookedPath.replace_extension(".bin");
			return cookedPath;
		}
	}

	void ParticleEffectSerializer::SerializeProperties(ryml::NodeRef node, const ParticleEffectProperties& properties)
	{
		node |= ryml::MAP;

		node["EmissionRate"] << properties.EmissionRate;
		node["Lifetime"] << properties.Lifetime;
		node["LifetimeVariation"] << properties.LifetimeVariation;

		Util::SerializeVector3f(node["Velocity"], properties.Velocity);
		Util::SerializeVector3f(node["VelocityVariation"], properties.VelocityVariation);
		node["GravityMultiplier"] << properties.GravityMultiplier;
		node["Drag"] << properties.Drag;

		node["AngularVelocity"] << properties.AngularVelocity;
		node["AngularVelocityVariation"] << properties.AngularVelocityVariation;
		node["AlignWithVelocity"] << properties.AlignWithVelocity;
		node["StretchFactor"] << properties.StretchFactor;

		Util::SerializeVector4f(node["ColorBegin"], properties.ColorBegin);
		Util::SerializeVector4f(node["ColorEnd"], properties.ColorEnd);
		node["ColorIntensity"] << properties.ColorIntensity;

		node["ScaleBegin"] << properties.ScaleBegin;
		node["ScaleEnd"] << properties.ScaleEnd;
		node["ScaleVariation"] << properties.ScaleVariation;

		node["TextureHandle"] << (uint64_t)properties.TextureHandle;
		node["BlendMode"] << ParticleBlendModeToString(properties.BlendMode);
	}

	void ParticleEffectSerializer::DeserializeProperties(ryml::NodeRef node, ParticleEffectProperties& properties)
	{
		Util::ReadField(node, "EmissionRate", properties.EmissionRate);
		Util::ReadField(node, "Lifetime", properties.Lifetime);
		Util::ReadField(node, "LifetimeVariation", properties.LifetimeVariation);

		Util::ReadVector3f(node, "Velocity", properties.Velocity);
		Util::ReadVector3f(node, "VelocityVariation", properties.VelocityVariation);
		Util::ReadField(node, "GravityMultiplier", properties.GravityMultiplier);
		Util::ReadField(node, "Drag", properties.Drag);

		Util::ReadField(node, "AngularVelocity", properties.AngularVelocity);
		Util::ReadField(node, "AngularVelocityVariation", properties.AngularVelocityVariation);
		Util::ReadField(node, "AlignWithVelocity", properties.AlignWithVelocity);
		Util::ReadField(node, "StretchFactor", properties.StretchFactor);

		Util::ReadVector4f(node, "ColorBegin", properties.ColorBegin);
		Util::ReadVector4f(node, "ColorEnd", properties.ColorEnd);
		Util::ReadField(node, "ColorIntensity", properties.ColorIntensity);

		Util::ReadField(node, "ScaleBegin", properties.ScaleBegin);
		Util::ReadField(node, "ScaleEnd", properties.ScaleEnd);
		Util::ReadField(node, "ScaleVariation", properties.ScaleVariation);

		uint64_t textureHandle = (uint64_t)properties.TextureHandle;
		Util::ReadField(node, "TextureHandle", textureHandle);
		properties.TextureHandle = (UUID)textureHandle;

		std::string blendMode = ParticleBlendModeToString(properties.BlendMode);
		Util::ReadField(node, "BlendMode", blendMode);
		properties.BlendMode = ParticleBlendModeFromString(blendMode);
	}

	bool ParticleEffectSerializer::SerializeSource(const std::filesystem::path& filepath, const SharedPtr<ParticleEffect>& effect)
	{
		if (!effect)
			return false;

		ryml::Tree tree;
		ryml::NodeRef root = tree.rootref();
		root |= ryml::MAP;
		root["Version"] << PARTICLE_EFFECT_SOURCE_FILE_VERSION;

		root["ParticleEffect"] << effect->GetName();
		root["UUID"] << (uint64_t)effect->GetUUID();

		SerializeProperties(root["Properties"], effect->Properties);

		std::ofstream fout(filepath);
		if (!fout.is_open())
		{
			EB_CORE_ERROR("Failed to open particle effect file for writing: {0}", filepath.string());
			return false;
		}

		fout << tree;
		fout.close();
		return true;
	}

	SharedPtr<ParticleEffect> ParticleEffectSerializer::DeserializeSource(UUID uuid, const std::filesystem::path& filepath)
	{
		std::ifstream stream(filepath);
		if (!stream.is_open())
		{
			EB_CORE_ERROR("Failed to open particle effect file: {0}", filepath.string());
			return nullptr;
		}

		std::stringstream strStream;
		strStream << stream.rdbuf();
		std::string yamlData = strStream.str();
		stream.close();

		ryml::Tree tree = ryml::parse_in_arena(ryml::to_csubstr(yamlData));
		ryml::NodeRef root = tree.rootref();

		std::string name = filepath.stem().string();
		Util::ReadField(root, "ParticleEffect", name);

		auto effect = SharedPtr<ParticleEffect>::Create(uuid, name, filepath.string());

		if (root.has_child("Properties"))
			DeserializeProperties(root["Properties"], effect->Properties);

		return effect;
	}

	bool ParticleEffectSerializer::SerializeCooked(const std::filesystem::path& filepath, const SharedPtr<ParticleEffect>& effect)
	{
		if (!effect)
			return false;

		auto cookedPath = GetCookedPath(filepath);
		std::ofstream stream(cookedPath, std::ios::binary | std::ios::trunc);
		if (!stream.is_open())
			return false;

		const auto& properties = effect->Properties;

		WriteRaw(stream, PARTICLE_EFFECT_COOKED_MAGIC);
		WriteRaw(stream, PARTICLE_EFFECT_COOKED_VERSION);

		WriteRaw(stream, properties.EmissionRate);
		WriteRaw(stream, properties.Lifetime);
		WriteRaw(stream, properties.LifetimeVariation);

		WriteRaw(stream, properties.Velocity);
		WriteRaw(stream, properties.VelocityVariation);
		WriteRaw(stream, properties.GravityMultiplier);
		WriteRaw(stream, properties.Drag);

		WriteRaw(stream, properties.AngularVelocity);
		WriteRaw(stream, properties.AngularVelocityVariation);
		WriteRaw(stream, properties.AlignWithVelocity);
		WriteRaw(stream, properties.StretchFactor);

		WriteRaw(stream, properties.ColorBegin);
		WriteRaw(stream, properties.ColorEnd);
		WriteRaw(stream, properties.ColorIntensity);

		WriteRaw(stream, properties.ScaleBegin);
		WriteRaw(stream, properties.ScaleEnd);
		WriteRaw(stream, properties.ScaleVariation);

		WriteRaw(stream, (uint64_t)properties.TextureHandle);
		WriteRaw(stream, (uint8_t)properties.BlendMode);
		return true;
	}

	SharedPtr<ParticleEffect> ParticleEffectSerializer::DeserializeCooked(UUID uuid, const std::filesystem::path& filepath)
	{
		std::ifstream stream(filepath, std::ios::binary);
		if (!stream.is_open())
			return nullptr;

		uint32_t magic = 0;
		uint32_t version = 0;
		if (!ReadRaw(stream, magic) || !ReadRaw(stream, version))
			return nullptr;
		if (magic != PARTICLE_EFFECT_COOKED_MAGIC || version > PARTICLE_EFFECT_COOKED_VERSION)
			return nullptr;

		auto effect = SharedPtr<ParticleEffect>::Create(uuid, filepath.stem().string(), filepath.string());
		auto& properties = effect->Properties;

		ReadRaw(stream, properties.EmissionRate);
		ReadRaw(stream, properties.Lifetime);
		ReadRaw(stream, properties.LifetimeVariation);

		ReadRaw(stream, properties.Velocity);
		ReadRaw(stream, properties.VelocityVariation);
		ReadRaw(stream, properties.GravityMultiplier);
		ReadRaw(stream, properties.Drag);

		ReadRaw(stream, properties.AngularVelocity);
		ReadRaw(stream, properties.AngularVelocityVariation);
		ReadRaw(stream, properties.AlignWithVelocity);
		ReadRaw(stream, properties.StretchFactor);

		ReadRaw(stream, properties.ColorBegin);
		ReadRaw(stream, properties.ColorEnd);
		ReadRaw(stream, properties.ColorIntensity);

		ReadRaw(stream, properties.ScaleBegin);
		ReadRaw(stream, properties.ScaleEnd);
		ReadRaw(stream, properties.ScaleVariation);

		uint64_t textureHandle = 0;
		ReadRaw(stream, textureHandle);
		properties.TextureHandle = (UUID)textureHandle;

		uint8_t blendMode = (uint8_t)ParticleBlendMode::Alpha;
		ReadRaw(stream, blendMode);
		properties.BlendMode = (ParticleBlendMode)blendMode;

		return effect;
	}

	bool ParticleEffectSerializer::Serialize(const std::filesystem::path& filepath, const SharedPtr<ParticleEffect>& effect)
	{
		return SerializeSource(filepath, effect);
	}

	SharedPtr<ParticleEffect> ParticleEffectSerializer::Deserialize(UUID uuid, const std::filesystem::path& filepath)
	{
		switch (AssetSerializationMode::GetRuntimeLoadTier())
		{
		case RuntimeAssetLoadTier::ForceSourceYaml:
			return DeserializeSource(uuid, filepath);
		case RuntimeAssetLoadTier::ForceCookedBinary:
			if (auto cooked = DeserializeCooked(uuid, GetCookedPath(filepath)))
				return cooked;
			return DeserializeSource(uuid, filepath);
		case RuntimeAssetLoadTier::Auto:
		default:
			if (filepath.extension() == ".bin")
			{
				if (auto cooked = DeserializeCooked(uuid, filepath))
					return cooked;
			}
			return DeserializeSource(uuid, filepath);
		}
	}

}
