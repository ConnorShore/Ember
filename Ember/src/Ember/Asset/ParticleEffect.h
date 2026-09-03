#pragma once

#include "Asset.h"

#include "Ember/Core/Core.h"
#include "Ember/Core/Constants.h"
#include "Ember/Math/Math.h"

namespace Ember {

	struct ParticleEffectProperties
	{
		float EmissionRate = 50.0f;

		float GravityMultiplier = 0.0f;

		Vector3f Velocity = { 0.0f, 1.0f, 0.0f };
		Vector3f VelocityVariation = { 0.5f, 0.1f, 0.5f };

		float Drag = 0.1f;
		float AngularVelocity = 0.0f;
		float AngularVelocityVariation = 0.2f;

		bool AlignWithVelocity = false;
		float StretchFactor = 0.0f;

		Vector4f ColorBegin = { 1.0f, 0.0f, 0.0f, 1.0f };
		Vector4f ColorEnd = { 0.0f, 0.0f, 1.0f, 0.0f };
		float ColorIntensity = 1.0f;

		float ScaleBegin = 1.0f;
		float ScaleEnd = 0.1f;
		float ScaleVariation = 0.3f;

		UUID TextureHandle = Constants::Assets::DefaultWhiteTexUUID;

		float Lifetime = 1.0f;
		float LifetimeVariation = 0.2f;
	};

	class ParticleEffect : public Asset
	{
	public:
		ParticleEffect(UUID uuid, const std::string& name, const std::string& filePath)
			: Asset(uuid, name, filePath, AssetType::ParticleEffect) {}
		ParticleEffect(UUID uuid, const std::string& name)
			: ParticleEffect(uuid, name, "") {}

		virtual ~ParticleEffect() = default;

		ParticleEffectProperties Properties;

		static AssetType GetStaticType() { return AssetType::ParticleEffect; }
	};

	class ParticleEffectInstance : public ParticleEffect
	{
	public:
		ParticleEffectInstance(UUID uuid, const std::string& name, const SharedPtr<ParticleEffect>& source)
			: ParticleEffect(uuid, name, ""), m_SourceEffect(source)
		{
			if (source)
				Properties = source->Properties;
		}
		ParticleEffectInstance(const std::string& name, const SharedPtr<ParticleEffect>& source)
			: ParticleEffectInstance(UUID(), name, source) {}

		virtual ~ParticleEffectInstance() = default;

		inline const SharedPtr<ParticleEffect>& GetSourceEffect() const { return m_SourceEffect; }
		inline void SetSourceEffect(const SharedPtr<ParticleEffect>& source) { m_SourceEffect = source; }

		inline void RevertToSource()
		{
			if (m_SourceEffect)
				Properties = m_SourceEffect->Properties;
		}

	private:
		SharedPtr<ParticleEffect> m_SourceEffect;
	};

}
