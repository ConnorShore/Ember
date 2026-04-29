#pragma once

#include "PostProcessPass.h"
#include "VFXTypes.h"

namespace Ember {

	class FogPass : public PostProcessPass
	{
	public:
		FogPass() = default;
		virtual ~FogPass() = default;

		virtual void Init() override;
		virtual void Render(PostProcessPassContext& context) override;
		inline virtual PostProcessStage GetStage() const override { return PostProcessStage::HDR; }

		FogSettings Settings;

	private:
		SharedPtr<Shader> m_FogShader;
	};

}