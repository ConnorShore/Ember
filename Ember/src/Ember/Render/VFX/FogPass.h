#pragma once

#include "PostProcessPass.h"

namespace Ember {

	class FogPass : public PostProcessPass
	{
	public:
		FogPass() = default;
		virtual ~FogPass() = default;

		virtual void Init() override;
		virtual void Render(PostProcessPassContext& context) override;
		inline virtual PostProcessStage GetStage() const override { return PostProcessStage::HDR; }

		// Fog variables here
		Vector3f Color = { 0.5f, 0.6f, 0.7f }; // Default greyish-blue
		float Density = 0.02f;
		float Falloff = 1.0f; // Useful if you add Height Fog later
		float StartDistance = 5.0f;

	private:
		SharedPtr<Shader> m_FogShader;
	};

}