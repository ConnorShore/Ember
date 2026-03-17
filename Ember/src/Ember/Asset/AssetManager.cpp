#include "ebpch.h"
#include "AssetManager.h"

namespace Ember {

	void AssetManager::LoadDefaults()
	{
		EB_CORE_INFO("Loading default engine assets...");

		// Textures
		uint32_t whiteTextureData = 0xffffffff;
		auto whiteTex = Create<Texture>(Constants::Assets::DefaultWhiteTex, 1, 1, &whiteTextureData);

		uint32_t flatNormalData = 0xffff8080;
		auto normalTex = Create<Texture>(Constants::Assets::DefaultNormalTex, 1, 1, &flatNormalData);

		uint32_t errorTextureData = 0xff00ffff;
		auto errorTex = Create<Texture>(Constants::Assets::DefaultErrorTex, 1, 1, &errorTextureData);

		// Shaders
		auto geometryShader = Load<Shader>(Constants::Assets::StandardGeometryShad, "Ember/assets/shaders/StandardGeometry.glsl");
		ShaderMacros lightMacros;
		lightMacros["MAX_DIRECTIONAL_LIGHTS"] = std::to_string(Constants::Renderer::MaxDirectionalLights);
		lightMacros["MAX_SPOT_LIGHTS"] = std::to_string(Constants::Renderer::MaxSpotLights);
		lightMacros["MAX_POINT_LIGHTS"] = std::to_string(Constants::Renderer::MaxPointLights);
		lightMacros["DEFAULT_AMBIENT"] = std::to_string(Constants::Renderer::DefaultAmbient);
		auto litShad = Load<Shader>(Constants::Assets::StandardLitShad, "Ember/assets/shaders/StandardLit.glsl", lightMacros);
		auto unlitShader = Load<Shader>(Constants::Assets::StandardUnlitShad, "Ember/assets/shaders/StandardUnlit.glsl");
		auto quadShader = Load<Shader>(Constants::Assets::Standard2dQuadShad, "Ember/assets/shaders/Renderer2D_Quad.glsl");
		auto shadowShader = Load<Shader>(Constants::Assets::StandardShadow, "Ember/assets/shaders/StandardShadow.glsl");

		// Materials
		auto geometryMaterial = Create<Material>(Constants::Assets::StandardGeometryMat, geometryShader, RenderQueue::Opaque);
		geometryMaterial->Set(Constants::Uniforms::Albedo, Vector3f(1.0f));
		geometryMaterial->Set(Constants::Uniforms::AlbedoMap, whiteTex);
		geometryMaterial->Set(Constants::Uniforms::Metallic, 0.0f);
		geometryMaterial->Set(Constants::Uniforms::Roughness, 0.5f);
		geometryMaterial->Set(Constants::Uniforms::AO, 1.0f);

		auto unlitMaterial = Create<Material>(Constants::Assets::StandardUnlitMat, unlitShader, RenderQueue::Forward);
		unlitMaterial->Set(Constants::Uniforms::Color, Vector3f(1.0f));

        auto fallbackMat = Create<MaterialInstance>(Constants::Assets::DefaultMat, geometryMaterial);
		fallbackMat->Set(Constants::Uniforms::Albedo, Vector3f(1.0f));
		fallbackMat->Set(Constants::Uniforms::Metallic, 0.0f);
		fallbackMat->Set(Constants::Uniforms::Roughness, 0.5f);
		fallbackMat->Set(Constants::Uniforms::AO, 1.0f);
		fallbackMat->Set(Constants::Uniforms::AlbedoMap, whiteTex);
		fallbackMat->Set(Constants::Uniforms::NormalMap, normalTex);
	}

    SharedPtr<Asset> AssetManager::GetAssetBase(UUID id) const
	{
		return m_Assets.at(id);
	}

}