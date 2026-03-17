#pragma once

namespace Ember::Constants {

	namespace Assets {
		inline const char* DefaultWhiteTex = "DefaultWhite";
		inline const char* DefaultErrorTex = "DefaultError";
		inline const char* DefaultNormalTex = "DefaultNormal";

		inline const char* StandardGeometryShad = "StandardGeometry";
		inline const char* StandardLitShad = "StandardLit";
		inline const char* StandardUnlitShad = "StandardUnlit";
		inline const char* Standard2dQuadShad = "Standard2DQuad";
		inline const char* StandardShadow = "StandardShadow";

		inline const char* DefaultMat = "DefaultMaterial";
		inline const char* StandardGeometryMat = "StandardGeometry";
		inline const char* StandardUnlitMat = "StandardUnlit";
	}

	namespace Uniforms {
		inline const char* Albedo = "u_Albedo";
		inline const char* Metallic = "u_Metallic";
		inline const char* Roughness = "u_Roughness";
		inline const char* AO = "u_AO";

		inline const char* AlbedoMap = "u_AlbedoMap";
		inline const char* NormalMap = "u_NormalMap";
		inline const char* MetallicMap = "u_MetallicMap";
		inline const char* RoughnessMap = "u_RoughnessMap";

		inline const char* Transform = "u_Transform";
		inline const char* ViewProj = "u_ViewProjection";
		inline const char* NormalMatrix = "u_NormalMatrix";

		inline const char* Color = "u_Color";

		inline const char* CameraPosition = "u_CameraPos";
		inline const char* ActiveDirectionalLights = "u_ActiveDirectionalLights";
		inline const char* ActivePointLights = "u_ActivePointLights";

		inline const char* LightViewMatrix = "u_LightViewMat";
	}

	namespace Renderer {
		inline constexpr unsigned int MaxDirectionalLights = 1;
		inline constexpr unsigned int MaxPointLights = 256;
		inline constexpr float DefaultAmbient = 0.03f;
	}

}