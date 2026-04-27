#pragma once

#include "PostProcessPass.h"

namespace Ember {

	class ToneMapPass : public PostProcessPass
	{
	public:
		ToneMapPass() = default;
		virtual ~ToneMapPass() = default;

		virtual void Init() override;
		virtual void Render(PostProcessPassContext& context) override;
		inline virtual PostProcessStage GetStage() const override { return PostProcessStage::ToneMap; }

		float Exposure = 1.0f;

	private:
		SharedPtr<Shader> m_ToneMapShader;
	};

}