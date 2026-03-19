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

	private:
		const unsigned int m_Passes = 10;

		SharedPtr<Shader> m_BlurShader;
		SharedPtr<Shader> m_BloomCompositeShader;

		std::array<SharedPtr<Framebuffer>, 2> m_PingPongBuffers;
	};

}