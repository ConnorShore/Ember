#pragma once

#include "PostProcessPass.h"
#include "VFXTypes.h"

namespace Ember {

	class VignettePass : public PostProcessPass
	{
	public:
		VignettePass() = default;
		virtual ~VignettePass() = default;

		virtual void Init() override;
		virtual void Render(PostProcessPassContext& context) override;
		inline virtual PostProcessStage GetStage() const override { return PostProcessStage::LDR; }

		VignetteSettings Settings;

	private:
		SharedPtr<Shader> m_VignetteShader;
	};

}