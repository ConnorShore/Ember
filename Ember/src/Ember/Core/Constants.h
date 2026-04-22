#pragma once

#include "Ember/Asset/UUID.h"

#include <cstdint>

namespace Ember::Constants {

	constexpr uint64_t InvalidUUID = 0;

	namespace Entities {
		constexpr uint64_t MaxEntities = 1024;
		constexpr uint64_t MaxComponents = 64;

		constexpr uint64_t InvalidEntityID = MaxEntities + 1;
		constexpr uint64_t InvalidComponentID = MaxEntities + 1;
	}

	namespace Renderer {
		inline constexpr uint32_t MaxDirectionalLights = 1;
		inline constexpr uint32_t MaxSpotLights = 1;
		inline constexpr uint32_t MaxPointLights = 256;

		inline constexpr uint32_t MaxBones = 100;
		inline constexpr uint32_t MaxParticles = 10000;

		inline constexpr float DefaultAmbient = 0.03f;
	}

	// TODO: Update sections to have massive regions 10000, 20000, 20001, etc
	namespace Assets {

		inline const char* DefaultWhiteTex = "DefaultWhite";
		inline const uint64_t DefaultWhiteTexUUID = 1;
		inline const char* DefaultErrorTex = "DefaultError";
		inline const uint64_t DefaultErrorTexUUID = 2;
		inline const char* DefaultNormalTex = "DefaultNormal";
		inline const uint64_t DefaultNormalTexUUID = 3;
		inline const char* DefaultBlackTex = "DefaultBlack";
		inline const uint64_t DefaultBlackTexUUID = 4;
		inline const char* DefaultSkybox = "DefaultSkybox";
		inline const uint64_t DefaultSkyboxUUID = 5;

		inline const char* StandardGeometryShad = "StandardGeometry";
		inline const uint64_t StandardGeometryShadUUID = 9;
		inline const char* StandardSkinnedGeometryShad = "StandardSkinnedGeometry";
		inline const uint64_t StandardSkinnedGeometryShadUUID = 10;
		inline const char* StandardLitShad = "StandardLit";
		inline const uint64_t StandardLitShadUUID = 11;
		inline const char* StandardUnlitShad = "StandardUnlit";
		inline const uint64_t StandardUnlitShadUUID = 12;
		inline const char* Standard2dQuadShad = "Standard2DQuad";
		inline const uint64_t Standard2dQuadShadUUID = 13;
		inline const char* StandardShadowShad = "StandardShadow";
		inline const uint64_t StandardShadowShadUUID = 14;
		inline const char* GaussianBlurShad = "GaussianBlur";
		inline const uint64_t GaussianBlurShadUUID = 15;
		inline const char* BloomShad = "Bloom";
		inline const uint64_t BloomShadUUID = 16;
		inline const char* OutlineShad = "Outline";
		inline const uint64_t OutlineShadUUID = 17;
		inline const char* FinalCompositeShad = "FinalComposite";
		inline const uint64_t FinalCompositeShadUUID = 18;
		inline const char* InfiniteGridShad = "InfiniteGrid";
		inline const uint64_t InfiniteGridShadUUID = 19;
		inline const char* BillboardShad = "Billboard";
		inline const uint64_t BillboardShadUUID = 20;
		inline const char* BloomPrefilterShad = "BloomPrefilter";
		inline const uint64_t BloomPrefilterShadUUID = 21;
		inline const char* SkyboxShad = "Skybox";
		inline const uint64_t SkyboxShadUUID = 22;
		inline const char* EquirectangularToCubemapShad = "EquirectangularToCubemap";
		inline const uint64_t EquirectangularToCubemapShadUUID = 23;
		inline const char* IrradianceShad = "Irradiance";
		inline const uint64_t IrradianceShadUUID = 24;
		inline const char* PrefilterShad = "PreFilter";
		inline const uint64_t PrefilterShadUUID = 25;
		inline const char* BRDFLUTShad = "BRDFLUT";
		inline const uint64_t BRDFLUTShadUUID = 26;
		inline const char* StandardSkinnedShadowShad = "StandardSkinnedShadow";
		inline const uint64_t StandardSkinnedShadowShadUUID = 27;
		inline const char* PhysicsDebugShad = "PhysicsDebug";
		inline const uint64_t PhysicsDebugShadUUID = 28;
		inline const char* FXAAShad = "FXAA";
		inline const uint64_t FXAAShadUUID = 29;
		inline const char* BlitShad = "Blit";
		inline const uint64_t BlitShadUUID = 1000;
		inline const char* ParticleShad = "Particle";
		inline const uint64_t ParticleShadUUID = 1001;

		inline const char* DefaultMat = "DefaultMaterial";
		inline const uint64_t DefaultMatUUID = 30;
		inline const char* StandardGeometryMat = "StandardGeometry";
		inline const uint64_t StandardGeometryMatUUID = 31;
		inline const char* StandardSkinnedGeometryMat = "StandardGeometrySkinned";
		inline const uint64_t StandardSkinnedGeometryMatUUID = 32;
		inline const char* StandardUnlitMat = "StandardUnlit";
		inline const uint64_t StandardUnlitMatUUID = 33;

		inline const uint64_t SphereMeshUUID = 50;
		inline const uint64_t CubeMeshUUID = 51;
		inline const uint64_t QuadMeshUUID = 52;
		inline const uint64_t CapsuleMeshUUID = 53;
	}

	namespace Uniforms {
		inline const char* Albedo = "u_Albedo";
		inline const char* Metallic = "u_Metallic";
		inline const char* Roughness = "u_Roughness";
		inline const char* AO = "u_AO";

		inline const char* AlbedoMap = "u_AlbedoMap";
		inline const char* NormalMap = "u_NormalMap";
		inline const char* MetallicRoughnessMap = "u_MetallicRoughnessMap";
		inline const char* EmissiveMap = "u_EmissiveMap";

		inline const char* Transform = "u_Transform";
		inline const char* Projection = "u_Projection";
		inline const char* View = "u_View";
		inline const char* ViewProj = "u_ViewProjection";
		inline const char* NormalMatrix = "u_NormalMatrix";
		inline const char* InverseView = "u_InverseView";
		inline const char* InverseProjection = "u_InverseProjection";

		inline const char* Image = "u_Image";
		inline const char* Color = "u_Color";
		inline const char* Emission = "u_Emission";
		inline const char* EmissionColor = "u_EmissionColor";
		inline const char* Scene = "u_Scene";
		inline const char* Exposure = "u_Exposure";

		inline const char* HorizontalPass = "u_HorizontalPass";
		inline const char* BloomBlur = "u_BloomBlur";
		inline const char* BlurRadius = "u_BlurRadius";
		inline const char* BloomIntensity = "u_BloomIntensity";
		inline const char* Threshold = "u_Threshold";
		inline const char* Knee = "u_Knee";

		inline const char* AlbedoRoughness = "u_AlbedoRoughness";
		inline const char* NormalMetallic = "u_NormalMetallic";
		inline const char* PositionAO = "u_PositionAO";
		inline const char* EmissionOut = "u_EmissionOut";
		inline const char* DirectionShadowMap = "u_DirectionShadowMap";
		inline const char* SpotShadowMap = "u_SpotShadowMap";

		inline const char* CameraPosition = "u_CameraPos";
		inline const char* ActiveDirectionalLights = "u_ActiveDirectionalLights";
		inline const char* ActiveSpotLights = "u_ActiveSpotLights";
		inline const char* ActivePointLights = "u_ActivePointLights";

		inline const char* LightViewMatrix = "u_LightViewMat";
		inline const char* DirectionalLightViewMatrix = "u_DirectionalLightViewMat";
		inline const char* SpotLightViewMatrix = "u_SpotLightViewMat";

		inline const char* EquirectangularMap = "u_EquirectangularMap";
		inline const char* EnvironmentMap = "u_EnvironmentMap";
		inline const char* IrradianceMap = "u_IrradianceMap";
		inline const char* PrefilterMap = "u_PrefilterMap";
		inline const char* BRDFLUT = "u_BRDFLUT";

		inline const char* InvViewportDimensions = "u_InvViewportDimensions";
		inline const char* EdgeThresholdMin = "u_EdgeThresholdMin";
		inline const char* EdgeThresholdMax = "u_EdgeThresholdMax";
		inline const char* SubpixelQuality = "u_SubpixelQuality";

		inline const char* BoneMatrices = "u_BoneMatrices";

		inline const char* EnvironmentIntensity = "u_EnvironmentIntensity";

		inline const char* EntityID = "u_EntityID";
	}

}