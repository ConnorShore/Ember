#pragma once

#include "PostProcessPass.h"

namespace Ember {

	class VignettePass : public PostProcessPass
	{
	public:
		VignettePass() = default;
		virtual ~VignettePass() = default;

		virtual void Init() override;
		virtual void Render(PostProcessPassContext& context) override;
		inline virtual PostProcessStage GetStage() const override { return PostProcessStage::LDR; }

		float Intensity = 1.0f;  // How dark the vignette is overall
		float Size = 0.5f;       // How far the clear center extends out
		float Smoothness = 0.5f; // How soft the fade is 
		Vector3f Color = { 0.0f, 0.0f, 0.0f };

	private:
		SharedPtr<Shader> m_VignetteShader;
	};

}