#include "ebpch.h"
#include "AssetManager.h"
#include "Ember/Render/PrimitiveGenerator.h"

namespace Ember {

	void AssetManager::LoadDefaults()
	{
		EB_CORE_INFO("Loading default engine assets...");

		// Textures
		uint32_t whiteTextureData = 0xffffffff;
		auto whiteTex = Create<Texture>(Constants::Assets::DefaultWhiteTexUUID, Constants::Assets::DefaultWhiteTex, 1, 1, &whiteTextureData);

		uint32_t flatNormalData = 0xffff8080;
		auto normalTex = Create<Texture>(Constants::Assets::DefaultNormalTexUUID, Constants::Assets::DefaultNormalTex, 1, 1, &flatNormalData);

		uint32_t errorTextureData = 0xff00ffff;
		auto errorTex = Create<Texture>(Constants::Assets::DefaultErrorTexUUID, Constants::Assets::DefaultErrorTex, 1, 1, &errorTextureData);

		// Shaders
		auto geometryShader = Load<Shader>(Constants::Assets::StandardGeometryShadUUID, Constants::Assets::StandardGeometryShad, "Ember/assets/shaders/StandardGeometry.glsl");
		ShaderMacros lightMacros;
		lightMacros["MAX_DIRECTIONAL_LIGHTS"] = std::to_string(Constants::Renderer::MaxDirectionalLights);
		lightMacros["MAX_SPOT_LIGHTS"] = std::to_string(Constants::Renderer::MaxSpotLights);
		lightMacros["MAX_POINT_LIGHTS"] = std::to_string(Constants::Renderer::MaxPointLights);
		lightMacros["DEFAULT_AMBIENT"] = std::to_string(Constants::Renderer::DefaultAmbient);
		auto litShad = Load<Shader>(Constants::Assets::StandardLitShadUUID, Constants::Assets::StandardLitShad, "Ember/assets/shaders/StandardLit.glsl", lightMacros);
		auto unlitShader = Load<Shader>(Constants::Assets::StandardUnlitShadUUID, Constants::Assets::StandardUnlitShad, "Ember/assets/shaders/StandardUnlit.glsl");
		auto quadShader = Load<Shader>(Constants::Assets::Standard2dQuadShadUUID, Constants::Assets::Standard2dQuadShad, "Ember/assets/shaders/Renderer2D_Quad.glsl");
		auto shadowShader = Load<Shader>(Constants::Assets::StandardShadowShadUUID, Constants::Assets::StandardShadowShad, "Ember/assets/shaders/StandardShadow.glsl");
		auto gaussianBlurShaer = Load<Shader>(Constants::Assets::GaussianBlurShadUUID, Constants::Assets::GaussianBlurShad, "Ember/assets/shaders/GaussianBlur.glsl");
		auto bloomShader = Load<Shader>(Constants::Assets::BloomShadUUID, Constants::Assets::BloomShad, "Ember/assets/shaders/Bloom.glsl");
		ShaderMacros outlineMacros;
		outlineMacros["INVALID_ENTITY_ID"] = std::to_string(Constants::Entities::InvalidEntityID);
		auto outlineShader = Load<Shader>(Constants::Assets::OutlineShad, "Ember/assets/shaders/Outline.glsl", outlineMacros);
		auto finalCompositeShader = Load<Shader>(Constants::Assets::FinalCompositeShad, "Ember/assets/shaders/FinalComposite.glsl");

		// Materials
		auto geometryMaterial = Create<Material>(Constants::Assets::StandardGeometryMatUUID, Constants::Assets::StandardGeometryMat, geometryShader, RenderQueue::Opaque);
		geometryMaterial->SetUniform(Constants::Uniforms::Albedo, Vector3f(1.0f));
		geometryMaterial->SetUniform(Constants::Uniforms::Metallic, 0.0f);
		geometryMaterial->SetUniform(Constants::Uniforms::Roughness, 0.5f);
		geometryMaterial->SetUniform(Constants::Uniforms::AO, 1.0f);
		geometryMaterial->SetUniform(Constants::Uniforms::AlbedoMap, whiteTex);
		geometryMaterial->SetUniform(Constants::Uniforms::NormalMap, normalTex);

		auto unlitMaterial = Create<Material>(Constants::Assets::StandardUnlitMatUUID, Constants::Assets::StandardUnlitMat, unlitShader, RenderQueue::Forward);
		unlitMaterial->SetUniform(Constants::Uniforms::Color, Vector3f(1.0f));
		unlitMaterial->SetUniform(Constants::Uniforms::Emission, 2.0f);

        auto fallbackMat = Create<MaterialInstance>(Constants::Assets::DefaultMatUUID, Constants::Assets::DefaultMat, geometryMaterial);
		fallbackMat->SetUniform(Constants::Uniforms::Albedo, Vector3f(1.0f));
		fallbackMat->SetUniform(Constants::Uniforms::Metallic, 0.0f);
		fallbackMat->SetUniform(Constants::Uniforms::Roughness, 0.5f);
		fallbackMat->SetUniform(Constants::Uniforms::AO, 1.0f);
		fallbackMat->SetUniform(Constants::Uniforms::AlbedoMap, whiteTex);
		fallbackMat->SetUniform(Constants::Uniforms::NormalMap, normalTex);

		// Basic Meshes
		auto sphereMesh = PrimitiveGenerator::CreateSphere();
		auto cubeMesh = PrimitiveGenerator::CreateCube();
		auto quadMesh = PrimitiveGenerator::CreateQuad();
		Register(Constants::Assets::SphereMeshUUID, sphereMesh);
		Register(Constants::Assets::CubeMeshUUID, cubeMesh);
		Register(Constants::Assets::QuadMeshUUID, quadMesh);
	}

    SharedPtr<Asset> AssetManager::GetAssetBase(UUID id) const
	{
		return m_Assets.at(id);
	}

}