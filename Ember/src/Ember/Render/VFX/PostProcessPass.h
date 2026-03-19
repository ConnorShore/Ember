#pragma once

#include "Ember/Core/Core.h"
#include "Ember/Render/Framebuffer.h"
#include "Ember/Render/PrimitiveGenerator.h"
#include "Ember/Render/Mesh.h"

namespace Ember {

	class PostProcessPass : public SharedResource
	{
	public:
		virtual ~PostProcessPass() = default;
		virtual void Init() = 0;
		virtual void Render(SharedPtr<Framebuffer> inputBuffer, SharedPtr<Framebuffer> outputBuffer) = 0;
		virtual void OnViewportResize(unsigned int width, unsigned int height) {}

		bool Enabled = true;

	protected:
		SharedPtr<Mesh> m_ScreenQuad = PrimitiveGenerator::CreateQuad(2.0f, 2.0f);
	};

}