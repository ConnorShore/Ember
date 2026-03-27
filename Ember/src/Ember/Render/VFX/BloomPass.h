#include "PostProcessPass.h"
#include "Ember/Render/Shader.h"

#include <array>

namespace Ember {

	class BloomPass : public PostProcessPass
	{
	public:
		BloomPass() = default;
		virtual ~BloomPass() = default;

		virtual void Init() override;
		virtual void Render(SharedPtr<Framebuffer> inputBuffer, SharedPtr<Framebuffer> outputBuffer) override;
		virtual void OnViewportResize(unsigned int width, unsigned int height) override;

	public:
		float Threshold = 1.5f;
		float Knee = 0.15f;
		float Intensity = 0.8f;
		float BlurRadius = 0.7f;

	private:
		const unsigned int m_Passes = 10;

		SharedPtr<Shader> m_BloomPrefilterShader;
		SharedPtr<Shader> m_BlurShader;
		SharedPtr<Shader> m_BloomCompositeShader;

		SharedPtr<Framebuffer> m_BloomExtractionBuffer;
		std::array<SharedPtr<Framebuffer>, 2> m_PingPongBuffers;
	};

}