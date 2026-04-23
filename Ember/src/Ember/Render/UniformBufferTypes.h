#pragma once

#include "Ember/Math/Math.h"
#include "Ember/Core/Constants.h"

namespace Ember {

	struct DirectionalLightData
	{
		Vector3f Direction; // 12 bytes
		float Intensity;    // 4 bytes

		Vector3f Color;     // 12 bytes
		float _Padding;     // 4 bytes
	};

	struct SpotLightData
	{
		Vector3f Position;  // 12 bytes
		float Intensity;    // 4 bytes

		Vector3f Direction; // 12 bytes
		float CutOff;       // 4 bytes

		Vector3f Color;     // 12 bytes
		float OuterCutOff;  // 4 bytes
	};

	struct PointLightData
	{
		Vector3f Position;  // 12 bytes
		float Intensity;    // 4 bytes

		Vector3f Color;     // 12 bytes
		float Radius;       // 4 bytes
	};

	// Main block matches shader layout
	struct LightDataBlock
	{
		DirectionalLightData DirectionalLights[Constants::Renderer::MaxDirectionalLights];
		SpotLightData SpotLights[Constants::Renderer::MaxSpotLights];
		PointLightData PointLights[Constants::Renderer::MaxPointLights];

		int ActiveDirectionalLights;
		int ActiveSpotLights;
		int ActivePointLights;
		int _Padding; // Pad the final ints to 16 bytes
	};

	struct ShadowDataBlock
	{
		// The 3 matrices for cascades (64 bytes * 3 = 192 bytes)
		Matrix4f DirectionalShadowMatrices[3];

		// The SpotLight matrix (64 bytes)
		Matrix4f SpotLightMatrix;

		// x = Cascade 0 Split, y = Cascade 1 Split, z = Cascade 2 Split (16 bytes)
		Vector4f CascadeSplits;
	};
}