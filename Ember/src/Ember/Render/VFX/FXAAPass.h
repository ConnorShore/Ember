#pragma once

#include "PostProcessPass.h"
#include "Ember/Render/Shader.h"

namespace Ember {

	class FXAAPass : public PostProcessPass
	{
	public:
		FXAAPass() = default;
		virtual ~FXAAPass() = default;

		virtual void Init() override;
		virtual void Render(SharedPtr<Framebuffer> inputBuffer, SharedPtr<Framebuffer> outputBuffer) override;

		inline virtual PostProcessStage GetStage() const override { return PostProcessStage::LDR; }

		inline void OnViewportResize(uint32_t width, uint32_t height) override
		{
			m_InvViewportDimensions = Vector2f(1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height));
		}

	public:
		float EdgeThresholdMin = 0.0312;	// 0.0 - 0.0833
		float EdgeThresholdMax = 0.125f;	// 0.063 - 0.333
		float SubpixelQuality = 0.75f;		// 0 - 1

	private:
		SharedPtr<Shader> m_FXAAShader;
		Vector2f m_InvViewportDimensions;
	};

}