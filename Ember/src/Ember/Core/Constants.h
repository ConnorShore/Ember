#pragma once

#include <sol/sol.hpp>

namespace Ember::Constants {

	constexpr unsigned int InvalidUUID = 0;

	namespace Entities {
		constexpr unsigned int MaxEntities = 1024;
		constexpr unsigned int MaxComponents = 64;

		constexpr unsigned int InvalidEntityID = MaxEntities + 1;
		constexpr unsigned int InvalidComponentID = MaxEntities + 1;
	}

	namespace Renderer {
		inline constexpr unsigned int MaxDirectionalLights = 1;
		inline constexpr unsigned int MaxSpotLights = 1;
		inline constexpr unsigned int MaxPointLights = 256;
		inline constexpr float DefaultAmbient = 0.03f;
	}

	namespace Assets {

		inline const char* DefaultWhiteTex = "DefaultWhite";
		inline const uint64_t DefaultWhiteTexUUID = 1;
		inline const char* DefaultErrorTex = "DefaultError";
		inline const uint64_t DefaultErrorTexUUID = 2;
		inline const char* DefaultNormalTex = "DefaultNormal";
		inline const uint64_t DefaultNormalTexUUID = 3;
		inline const char* DefaultBlackTex = "DefaultBlack";
		inline const uint64_t DefaultBlackTexUUID = 4;

		inline const char* StandardGeometryShad = "StandardGeometry";
		inline const uint64_t StandardGeometryShadUUID = 10;
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

		inline const char* DefaultMat = "DefaultMaterial";
		inline const uint64_t DefaultMatUUID = 30;
		inline const char* StandardGeometryMat = "StandardGeometry";
		inline const uint64_t StandardGeometryMatUUID = 31;
		inline const char* StandardUnlitMat = "StandardUnlit";
		inline const uint64_t StandardUnlitMatUUID = 32;

		inline const uint64_t SphereMeshUUID = 50;
		inline const uint64_t CubeMeshUUID = 51;
		inline const uint64_t QuadMeshUUID = 52;
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
		inline const char* ViewProj = "u_ViewProjection";
		inline const char* NormalMatrix = "u_NormalMatrix";
		inline const char* InverseView = "u_InverseView";
		inline const char* InverseProjection = "u_InverseProjection";

		inline const char* Image = "u_Image";
		inline const char* Color = "u_Color";
		inline const char* Emission = "u_Emission";
		inline const char* Scene = "u_Scene";
		inline const char* Exposure = "u_Exposure";

		inline const char* HorizontalPass = "u_HorizontalPass";
		inline const char* BloomBlur = "u_BloomBlur";

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

		inline const char* EntityID = "u_EntityID";
	}

}