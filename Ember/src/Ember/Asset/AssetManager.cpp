#include "ebpch.h"
#include "AssetManager.h"
#include "Ember/Render/PrimitiveGenerator.h"
#include "Ember/Render/LUTGenerator.h"

#include <filesystem>

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

		auto defaultSkybox = Load<Texture2D>(Constants::Assets::DefaultSkyboxUUID, Constants::Assets::DefaultSkybox, (m_EngineAssetDirectory / "textures/DefaultSkybox.hdr").string());
		auto defaultNeutralLUT = Load<Texture2D>(Constants::Assets::DefaultNeutralColorLUTUUID, Constants::Assets::DefaultNeutralColorLUT, (m_EngineAssetDirectory / "textures/DefaultNeutralColorLUT.png").string());

		// Shaders
		// Fallback shader loaded first so any subsequent shader that fails to compile/link can
		// substitute it via Shader::ActiveProgram() and render solid pink instead of nothing.
		auto fallbackShader = Load<Shader>(Constants::Assets::FallbackShadUUID, Constants::Assets::FallbackShad, (m_EngineAssetDirectory / "shaders/Fallback.glsl").string());
		auto geometryShader = Load<Shader>(Constants::Assets::StandardGeometryShadUUID, Constants::Assets::StandardGeometryShad, (m_EngineAssetDirectory / "shaders/StandardGeometry.glsl").string());
		ShaderMacros skinGeoMacros;
		skinGeoMacros["MAX_BONES"] = std::to_string(Constants::Renderer::MaxBones);
		auto skinnedGeometryShader = Load<Shader>(Constants::Assets::StandardSkinnedGeometryShadUUID, Constants::Assets::StandardSkinnedGeometryShad, (m_EngineAssetDirectory / "shaders/StandardGeometrySkinned.glsl").string(), skinGeoMacros);
		ShaderMacros lightMacros;
		lightMacros["MAX_DIRECTIONAL_LIGHTS"] = std::to_string(Constants::Renderer::MaxDirectionalLights);
		lightMacros["MAX_SPOT_LIGHTS"] = std::to_string(Constants::Renderer::MaxSpotLights);
		lightMacros["MAX_POINT_LIGHTS"] = std::to_string(Constants::Renderer::MaxPointLights);
		lightMacros["DEFAULT_AMBIENT"] = std::to_string(Constants::Renderer::DefaultAmbient);
		auto litShad = Load<Shader>(Constants::Assets::StandardLitShadUUID, Constants::Assets::StandardLitShad, (m_EngineAssetDirectory / "shaders/StandardLit.glsl").string(), lightMacros);
		auto unlitShader = Load<Shader>(Constants::Assets::StandardUnlitShadUUID, Constants::Assets::StandardUnlitShad, (m_EngineAssetDirectory / "shaders/StandardUnlit.glsl").string());
		auto quadShader = Load<Shader>(Constants::Assets::Standard2dQuadShadUUID, Constants::Assets::Standard2dQuadShad, (m_EngineAssetDirectory / "shaders/Renderer2D_Quad.glsl").string());
		auto shadowShader = Load<Shader>(Constants::Assets::StandardShadowShadUUID, Constants::Assets::StandardShadowShad, (m_EngineAssetDirectory / "shaders/StandardShadow.glsl").string());
		ShaderMacros skinShadMacros;
		skinShadMacros["MAX_BONES"] = std::to_string(Constants::Renderer::MaxBones);
		auto shadowSkinnedShader = Load<Shader>(Constants::Assets::StandardSkinnedShadowShadUUID, Constants::Assets::StandardSkinnedShadowShad, (m_EngineAssetDirectory / "shaders/StandardSkinnedShadow.glsl").string(), skinShadMacros);
		auto gaussianBlurShaer = Load<Shader>(Constants::Assets::GaussianBlurShadUUID, Constants::Assets::GaussianBlurShad, (m_EngineAssetDirectory / "shaders/GaussianBlur.glsl").string());
		auto bloomShader = Load<Shader>(Constants::Assets::BloomShadUUID, Constants::Assets::BloomShad, (m_EngineAssetDirectory / "shaders/Bloom.glsl").string());
		ShaderMacros outlineMacros;
		outlineMacros["INVALID_ENTITY_ID"] = std::to_string(Constants::Entities::InvalidEntityID);
		auto outlineShader = Load<Shader>(Constants::Assets::OutlineShadUUID, Constants::Assets::OutlineShad, (m_EngineAssetDirectory / "shaders/Outline.glsl").string(), outlineMacros);
		auto toneMappingShader = Load<Shader>(Constants::Assets::ToneMapShadUUID, Constants::Assets::ToneMapShad, (m_EngineAssetDirectory / "shaders/ToneMapping.glsl").string());
		ShaderMacros infiniteGridMacros;
		infiniteGridMacros["INVALID_ENTITY_ID"] = std::to_string(Constants::Entities::InvalidEntityID);
		auto infiniteGridShader = Load<Shader>(Constants::Assets::InfiniteGridShadUUID, Constants::Assets::InfiniteGridShad, (m_EngineAssetDirectory / "shaders/InfiniteGrid.glsl").string(), infiniteGridMacros);
		auto billboardShader = Load<Shader>(Constants::Assets::BillboardShadUUID, Constants::Assets::BillboardShad, (m_EngineAssetDirectory / "shaders/Billboard.glsl").string());
		auto bloomPrefilter = Load<Shader>(Constants::Assets::BloomPrefilterShadUUID, Constants::Assets::BloomPrefilterShad, (m_EngineAssetDirectory / "shaders/BloomPrefilter.glsl").string());
		auto skyboxShader = Load<Shader>(Constants::Assets::SkyboxShadUUID, Constants::Assets::SkyboxShad, (m_EngineAssetDirectory / "shaders/Skybox.glsl").string());
		auto equirectangularToCubemapShad = Load<Shader>(Constants::Assets::EquirectangularToCubemapShadUUID, Constants::Assets::EquirectangularToCubemapShad, (m_EngineAssetDirectory / "shaders/EquirectangularToCubemap.glsl").string());
		auto irradianceShad = Load<Shader>(Constants::Assets::IrradianceShadUUID, Constants::Assets::IrradianceShad, (m_EngineAssetDirectory / "shaders/Irradiance.glsl").string());
		auto prefilterShad = Load<Shader>(Constants::Assets::PrefilterShadUUID, Constants::Assets::PrefilterShad, (m_EngineAssetDirectory / "shaders/Prefilter.glsl").string());
		auto brdfLUTShad = Load<Shader>(Constants::Assets::BRDFLUTShadUUID, Constants::Assets::BRDFLUTShad, (m_EngineAssetDirectory / "shaders/BRDF.glsl").string());
		auto physicsDebugShad = Load<Shader>(Constants::Assets::PhysicsDebugShadUUID, Constants::Assets::PhysicsDebugShad, (m_EngineAssetDirectory / "shaders/PhysicsDebug.glsl").string());
		auto fxaaShad = Load<Shader>(Constants::Assets::FXAAShadUUID, Constants::Assets::FXAAShad, (m_EngineAssetDirectory / "shaders/FXAA.glsl").string());
		auto blitShad = Load<Shader>(Constants::Assets::BlitShadUUID, Constants::Assets::BlitShad, (m_EngineAssetDirectory / "shaders/FinalBlit.glsl").string());
		auto particleShad = Load<Shader>(Constants::Assets::ParticleShadUUID, Constants::Assets::ParticleShad, (m_EngineAssetDirectory / "shaders/Particle.glsl").string());
		auto colorGradeEditorShad = Load<Shader>(Constants::Assets::ColorGradeEditorShadUUID, Constants::Assets::ColorGradeEditorShad, (m_EngineAssetDirectory / "shaders/ColorGradeEditor.glsl").string());
		auto colorGradeRuntimeShad = Load<Shader>(Constants::Assets::ColorGradeRuntimeShadUUID, Constants::Assets::ColorGradeRuntimeShad, (m_EngineAssetDirectory / "shaders/ColorGradeRuntime.glsl").string());
		auto fogShad = Load<Shader>(Constants::Assets::FogShadUUID, Constants::Assets::FogShad, (m_EngineAssetDirectory / "shaders/Fog.glsl").string());
		auto vignetteShad = Load<Shader>(Constants::Assets::VignetteShadUUID, Constants::Assets::VignetteShad, (m_EngineAssetDirectory / "shaders/Vignette.glsl").string());

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

		// Scripts
		//auto characterMovementScript = Load<Script>(Constants::Assets::CharacterMovementScriptUUID, Constants::Assets::CharacterMovementScript, "Ember/assets/scripts/CharacterMovement.lua");
		//auto freeLookScript = Load<Script>(Constants::Assets::FreeLookScriptUUID, Constants::Assets::FreeLookScript, "Ember/assets/scripts/FreeLook.lua");
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