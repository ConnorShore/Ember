#pragma once

#include "Ember/Core/Core.h"
#include "Ember/Render/Framebuffer.h"
#include "Ember/Render/PrimitiveGenerator.h"

namespace Ember {

	class PostProcessPass : public SharedResource
	{
	public:
		virtual ~PostProcessPass() = default;
		virtual void Init() = 0;
		virtual void Render(SharedPtr<Framebuffer> inputBuffer, SharedPtr<Framebuffer> outputBuffer) = 0;
		virtual void OnViewportResize(uint32_t width, uint32_t height) {}

		bool Enabled = true;

	protected:
		SharedPtr<StaticMesh> m_ScreenQuad = PrimitiveGenerator::CreateQuad(2.0f, 2.0f);
	};

}