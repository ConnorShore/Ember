#pragma once

#include "Ember/Core/Core.h"
#include "Ember/Render/Framebuffer.h"
#include "Ember/Render/PrimitiveGenerator.h"

namespace Ember {

	enum class PostProcessStage
	{
		HDR = 0, // Pre-Composite (Bloom, Lens Flares)
		LDR = 1  // Post-Composite (FXAA, Vignette, Film Grain)
	};

	class PostProcessPass : public SharedResource
	{
	public:
		virtual ~PostProcessPass() = default;
		virtual void Init() = 0;
		virtual void Render(SharedPtr<Framebuffer> inputBuffer, SharedPtr<Framebuffer> outputBuffer) = 0;
		virtual void OnViewportResize(uint32_t width, uint32_t height) {}

		virtual PostProcessStage GetStage() const = 0;

		bool Enabled = true;

	protected:
		SharedPtr<StaticMesh> m_ScreenQuad = PrimitiveGenerator::CreateQuad(2.0f, 2.0f);
	};

}