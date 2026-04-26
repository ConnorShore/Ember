#pragma once

#include "Ember/Math/Math.h"

namespace Ember {

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

}