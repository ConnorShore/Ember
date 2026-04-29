#pragma once

#include "PostProcessPass.h"
#include "VFXTypes.h"

namespace Ember {

	class ToneMapPass : public PostProcessPass
	{
	public:
		ToneMapPass() = default;
		virtual ~ToneMapPass() = default;

		virtual void Init() override;
		virtual void Render(PostProcessPassContext& context) override;
		inline virtual PostProcessStage GetStage() const override { return PostProcessStage::ToneMap; }

		ToneMapSettings Settings;

	private:
		SharedPtr<Shader> m_ToneMapShader;
	};

}