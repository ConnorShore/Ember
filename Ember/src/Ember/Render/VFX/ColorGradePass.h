#pragma once

#include "PostProcessPass.h"
#include "VFXTypes.h"

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

		inline void SetBakedLUT(const SharedPtr<Framebuffer>& colorGradeLUTBuffer) { m_ColorGradeLUTBuffer = colorGradeLUTBuffer; }
		inline void SetBaseBakedLUT(const SharedPtr<Texture2D>& baseBakedLUTTexture) { m_BaseBakedLUTTexture = baseBakedLUTTexture; }

		inline const SharedPtr<Texture2D>& GetBaseBakedLUT() const { return m_BaseBakedLUTTexture; }

		ColorGradeSettings Settings;

	private:
		void RenderEditor(SharedPtr<Framebuffer> inputBuffer, SharedPtr<Framebuffer> outputBuffer);
		void RenderRuntime(SharedPtr<Framebuffer> inputBuffer, SharedPtr<Framebuffer> outputBuffer);

	private:
		SharedPtr<Shader> m_ColorGradeShaderEditor;
		SharedPtr<Shader> m_ColorGradeShaderRuntime;
		SharedPtr<Framebuffer> m_ColorGradeLUTBuffer;
		SharedPtr<Texture2D> m_BaseBakedLUTTexture;
	};

}