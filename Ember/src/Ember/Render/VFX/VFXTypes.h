#pragma once

#include "Ember/Math/Math.h"

namespace Ember {

	struct BloomSettings
	{
		float Threshold = 1.5f;
		float Knee = 0.15f;
		float Intensity = 0.8f;
		float BlurRadius = 0.7f;
	};

	struct ColorGradeSettings
	{
		// TODO: Probably at some point want to break out different color grade settings to their own passes
		float Temperature = 0.0f;
		float Tint = 0.0f;

		float Contrast = 1.0f;
		float Saturation = 1.0f;

		Vector4f Lift = { 0.0f, 0.0f, 0.0f, 0.0f };
		Vector4f Gamma = { 1.0f, 1.0f, 1.0f, 0.0f };
		Vector4f Gain = { 1.0f, 1.0f, 1.0f, 0.0f };

		// TODO: Add UUID for LUT texture and logic for baking/saving LUTs on a per-volume basis

		void Reset()
		{
			Temperature = 0.0f;
			Tint = 0.0f;
			Contrast = 1.0f;
			Saturation = 1.0f;
			Lift = { 0.0f, 0.0f, 0.0f, 0.0f };
			Gamma = { 1.0f, 1.0f, 1.0f, 0.0f };
			Gain = { 1.0f, 1.0f, 1.0f, 0.0f };
		}
	};

	struct FogSettings
	{
		Vector3f Color = { 0.5f, 0.6f, 0.7f }; // Default greyish-blue
		float Density = 0.02f;
		float Falloff = 1.0f; // Useful if you add Height Fog later
		float StartDistance = 5.0f;
	};

	struct VignetteSettings
	{
		float Intensity = 1.0f;  // How dark the vignette is overall
		float Size = 0.5f;       // How far the clear center extends out
		float Smoothness = 0.5f; // How soft the fade is 
		Vector3f Color = { 0.0f, 0.0f, 0.0f };
	};

	struct ToneMapSettings
	{
		float Exposure = 1.0f;
	};

	struct PostProcessVolumeSettings
	{
		bool BloomEnabled = false;
		bool ColorGradeEnabled = false;
		bool FogEnabled = false;
		bool VignetteEnabled = false;

		BloomSettings Bloom;
		ColorGradeSettings ColorGrade;
		FogSettings Fog;
		VignetteSettings Vignette;
		ToneMapSettings ToneMap;	// For exposure
	};
}