#pragma once

#include "PostProcessPass.h"

namespace Ember {

	class Shader;

	class ColorGradePass : public PostProcessPass
	{
	public:
		ColorGradePass() = default;
		virtual ~ColorGradePass() = default;

		virtual void Init() override;
		virtual void Render(PostProcessPassContext& context) override;

		inline virtual PostProcessStage GetStage() const override { return PostProcessStage::LDR; }

	public:
		struct ColorGradeSettings
		{
			// TODO: Probably at some point want to break out different color grade settings to their own passes
			float Temperature = 0.0f;
			float Tint = 0.0f;

			float Contrast = 1.0f;
			float Saturation = 1.0f;

			Vector4f Lift = { 1.0f, 1.0f, 1.0f, 0.0f };
			Vector4f Gamma = { 1.0f, 1.0f, 1.0f, 0.0f };
			Vector4f Gain = { 1.0f, 1.0f, 1.0f, 0.0f };
		};

		ColorGradeSettings Settings;

	private:
		void RenderEditor(SharedPtr<Framebuffer> inputBuffer, SharedPtr<Framebuffer> outputBuffer);
		void RenderRuntime(SharedPtr<Framebuffer> inputBuffer, SharedPtr<Framebuffer> outputBuffer);

	private:
		SharedPtr<Shader> m_ColorGradeShaderEditor;
		SharedPtr<Shader> m_ColorGradeShaderRuntime;
	};

}