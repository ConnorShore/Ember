#include "ebpch.h"
#include "AssetManager.h"
#include "Ember/Render/PrimitiveGenerator.h"

namespace Ember {

	void AssetManager::LoadDefaults()
	{
		// TODO: Clean this up and split out defaults to different sections (i.e. LoadDefaultMeshes, LoadDefaultTextures, etc.)
		EB_CORE_INFO("Loading default engine assets...");

		// Textures
		uint32_t whiteTextureData = 0xffffffff;
		auto whiteTex = Create<Texture2D>(Constants::Assets::DefaultWhiteTexUUID, Constants::Assets::DefaultWhiteTex, 1, 1, &whiteTextureData);

		uint32_t flatNormalData = 0xffff8080;
		auto normalTex = Create<Texture2D>(Constants::Assets::DefaultNormalTexUUID, Constants::Assets::DefaultNormalTex, 1, 1, &flatNormalData);

		uint32_t errorTextureData = 0xff00ffff;
		auto errorTex = Create<Texture2D>(Constants::Assets::DefaultErrorTexUUID, Constants::Assets::DefaultErrorTex, 1, 1, &errorTextureData);

		uint32_t blackTextureData = 0x00000000;
		auto blackTex = Create<Texture2D>(Constants::Assets::DefaultBlackTexUUID, Constants::Assets::DefaultBlackTex, 1, 1, &blackTextureData);

		auto defaultSkybox = Load<Texture2D>(Constants::Assets::DefaultSkyboxUUID, Constants::Assets::DefaultSkybox, "Ember/assets/textures/DefaultSkybox.hdr");

		// Shaders
		auto geometryShader = Load<Shader>(Constants::Assets::StandardGeometryShadUUID, Constants::Assets::StandardGeometryShad, "Ember/assets/shaders/StandardGeometry.glsl");
		ShaderMacros skinGeoMacros;
		skinGeoMacros["MAX_BONES"] = std::to_string(Constants::Renderer::MaxBones);
		auto skinnedGeometryShader = Load<Shader>(Constants::Assets::StandardSkinnedGeometryShadUUID, Constants::Assets::StandardSkinnedGeometryShad, "Ember/assets/shaders/StandardGeometrySkinned.glsl", skinGeoMacros);
		ShaderMacros lightMacros;
		lightMacros["MAX_DIRECTIONAL_LIGHTS"] = std::to_string(Constants::Renderer::MaxDirectionalLights);
		lightMacros["MAX_SPOT_LIGHTS"] = std::to_string(Constants::Renderer::MaxSpotLights);
		lightMacros["MAX_POINT_LIGHTS"] = std::to_string(Constants::Renderer::MaxPointLights);
		lightMacros["DEFAULT_AMBIENT"] = std::to_string(Constants::Renderer::DefaultAmbient);
		auto litShad = Load<Shader>(Constants::Assets::StandardLitShadUUID, Constants::Assets::StandardLitShad, "Ember/assets/shaders/StandardLit.glsl", lightMacros);
		auto unlitShader = Load<Shader>(Constants::Assets::StandardUnlitShadUUID, Constants::Assets::StandardUnlitShad, "Ember/assets/shaders/StandardUnlit.glsl");
		auto quadShader = Load<Shader>(Constants::Assets::Standard2dQuadShadUUID, Constants::Assets::Standard2dQuadShad, "Ember/assets/shaders/Renderer2D_Quad.glsl");
		auto shadowShader = Load<Shader>(Constants::Assets::StandardShadowShadUUID, Constants::Assets::StandardShadowShad, "Ember/assets/shaders/StandardShadow.glsl");
		ShaderMacros skinShadMacros;
		skinShadMacros["MAX_BONES"] = std::to_string(Constants::Renderer::MaxBones);
		auto shadowSkinnedShader = Load<Shader>(Constants::Assets::StandardSkinnedShadowShadUUID, Constants::Assets::StandardSkinnedShadowShad, "Ember/assets/shaders/StandardSkinnedShadow.glsl", skinShadMacros);
		auto gaussianBlurShaer = Load<Shader>(Constants::Assets::GaussianBlurShadUUID, Constants::Assets::GaussianBlurShad, "Ember/assets/shaders/GaussianBlur.glsl");
		auto bloomShader = Load<Shader>(Constants::Assets::BloomShadUUID, Constants::Assets::BloomShad, "Ember/assets/shaders/Bloom.glsl");
		ShaderMacros outlineMacros;
		outlineMacros["INVALID_ENTITY_ID"] = std::to_string(Constants::Entities::InvalidEntityID);
		auto outlineShader = Load<Shader>(Constants::Assets::OutlineShad, "Ember/assets/shaders/Outline.glsl", outlineMacros);
		auto finalCompositeShader = Load<Shader>(Constants::Assets::FinalCompositeShad, "Ember/assets/shaders/FinalComposite.glsl");
		ShaderMacros infiniteGridMacros;
		infiniteGridMacros["INVALID_ENTITY_ID"] = std::to_string(Constants::Entities::InvalidEntityID);
		auto infiniteGridShader = Load<Shader>(Constants::Assets::InfiniteGridShad, "Ember/assets/shaders/InfiniteGrid.glsl", infiniteGridMacros);
		auto billboardShader = Load<Shader>(Constants::Assets::BillboardShad, "Ember/assets/shaders/Billboard.glsl");
		auto bloomPrefilter = Load<Shader>(Constants::Assets::BloomPrefilterShadUUID, Constants::Assets::BloomPrefilterShad, "Ember/assets/shaders/BloomPrefilter.glsl");
		auto skyboxShader = Load<Shader>(Constants::Assets::SkyboxShadUUID, Constants::Assets::SkyboxShad, "Ember/assets/shaders/Skybox.glsl");
		auto equirectangularToCubemapShad = Load<Shader>(Constants::Assets::EquirectangularToCubemapShadUUID, Constants::Assets::EquirectangularToCubemapShad, "Ember/assets/shaders/EquirectangularToCubemap.glsl");
		auto irradianceShad = Load<Shader>(Constants::Assets::IrradianceShadUUID, Constants::Assets::IrradianceShad, "Ember/assets/shaders/Irradiance.glsl");
		auto prefilterShad = Load<Shader>(Constants::Assets::PrefilterShadUUID, Constants::Assets::PrefilterShad, "Ember/assets/shaders/Prefilter.glsl");
		auto brdfLUTShad = Load<Shader>(Constants::Assets::BRDFLUTShadUUID, Constants::Assets::BRDFLUTShad, "Ember/assets/shaders/BRDF.glsl");
		auto physicsDebugShad = Load<Shader>(Constants::Assets::PhysicsDebugShadUUID, Constants::Assets::PhysicsDebugShad, "Ember/assets/shaders/PhysicsDebug.glsl");
		auto fxaaShad = Load<Shader>(Constants::Assets::FXAAShadUUID, Constants::Assets::FXAAShad, "Ember/assets/shaders/FXAA.glsl");
		auto blitShad = Load<Shader>(Constants::Assets::BlitShadUUID, Constants::Assets::BlitShad, "Ember/assets/shaders/FinalBlit.glsl");
		ShaderMacros particleShadMacros;
		particleShadMacros["MAX_PARTICLES"] = std::to_string(Constants::Renderer::MaxParticles);
		auto particleShad = Load<Shader>(Constants::Assets::ParticleShadUUID, Constants::Assets::ParticleShad, "Ember/assets/shaders/Particles.glsl", particleShadMacros);

		// Materials
		auto geometryMaterial = Create<Material>(Constants::Assets::StandardGeometryMatUUID, Constants::Assets::StandardGeometryMat, geometryShader, RenderQueue::Opaque);
		geometryMaterial->SetUniform(Constants::Uniforms::Albedo, Vector3f(0.75f));
		geometryMaterial->SetUniform(Constants::Uniforms::Metallic, 0.0f);
		geometryMaterial->SetUniform(Constants::Uniforms::Roughness, 0.5f);
		geometryMaterial->SetUniform(Constants::Uniforms::AO, 1.0f);
		geometryMaterial->SetUniform(Constants::Uniforms::Emission, 0.0f);
		geometryMaterial->SetUniform(Constants::Uniforms::EmissionColor, Vector3f(1.0f));
		geometryMaterial->SetUniform(Constants::Uniforms::AlbedoMap, whiteTex);
		geometryMaterial->SetUniform(Constants::Uniforms::NormalMap, normalTex);
		geometryMaterial->SetUniform(Constants::Uniforms::MetallicRoughnessMap, whiteTex);
		geometryMaterial->SetUniform(Constants::Uniforms::EmissiveMap, whiteTex);

		auto geometrySkinnedMaterial = Create<Material>(Constants::Assets::StandardSkinnedGeometryMatUUID, Constants::Assets::StandardSkinnedGeometryMat, skinnedGeometryShader, RenderQueue::Opaque);
		geometrySkinnedMaterial->SetUniform(Constants::Uniforms::Albedo, Vector3f(0.75f));
		geometrySkinnedMaterial->SetUniform(Constants::Uniforms::Metallic, 0.0f);
		geometrySkinnedMaterial->SetUniform(Constants::Uniforms::Roughness, 0.5f);
		geometrySkinnedMaterial->SetUniform(Constants::Uniforms::AO, 1.0f);
		geometrySkinnedMaterial->SetUniform(Constants::Uniforms::Emission, 0.0f);
		geometrySkinnedMaterial->SetUniform(Constants::Uniforms::EmissionColor, Vector3f(1.0f));
		geometrySkinnedMaterial->SetUniform(Constants::Uniforms::AlbedoMap, whiteTex);
		geometrySkinnedMaterial->SetUniform(Constants::Uniforms::NormalMap, normalTex);
		geometrySkinnedMaterial->SetUniform(Constants::Uniforms::MetallicRoughnessMap, whiteTex);
		geometrySkinnedMaterial->SetUniform(Constants::Uniforms::EmissiveMap, whiteTex);

		auto unlitMaterial = Create<Material>(Constants::Assets::StandardUnlitMatUUID, Constants::Assets::StandardUnlitMat, unlitShader, RenderQueue::Forward);
		unlitMaterial->SetUniform(Constants::Uniforms::Color, Vector3f(1.0f));
		unlitMaterial->SetUniform(Constants::Uniforms::Emission, 2.0f);

		auto fallbackMat = Create<MaterialInstance>(Constants::Assets::DefaultMatUUID, Constants::Assets::DefaultMat, geometryMaterial);

		// Basic Meshes
		auto sphereMesh = PrimitiveGenerator::CreateSphere();
		auto cubeMesh = PrimitiveGenerator::CreateCube();
		auto quadMesh = PrimitiveGenerator::CreateQuad();
		auto capsuleMesh = PrimitiveGenerator::CreateCapsule();
		Register(Constants::Assets::SphereMeshUUID, sphereMesh);
		Register(Constants::Assets::CubeMeshUUID, cubeMesh);
		Register(Constants::Assets::QuadMeshUUID, quadMesh);
		Register(Constants::Assets::CapsuleMeshUUID, capsuleMesh);
	}
	
	void AssetManager::ClearAssets()
	{
		// Clear all assets except engine assets
		for (auto it = m_AssetNames.begin(); it != m_AssetNames.end(); )
		{
			auto name = it->first;
			auto uuid = it->second;
			auto asset = m_Assets.at(uuid);
			if (!asset->IsEngineAsset())
			{
				it = m_AssetNames.erase(it);
				m_AssetPaths.erase(asset->GetFilePath());
				m_Assets.erase(uuid);
			}
			else 
			{
				it++;
			}
		}
	}

	SharedPtr<Asset> AssetManager::GetAssetBase(UUID id) const
	{
		return m_Assets.at(id);
	}

}