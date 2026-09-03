#pragma once

#include "RenderPass.h"

#include "Ember/Render/Particle.h"
#include "Ember/Render/RendererAPI.h"

#include <vector>
#include <array>

namespace Ember {

	class ParticleManager;

	class ParticleRenderPass : public RenderPass
	{
	public:
		ParticleRenderPass() = default;
		virtual ~ParticleRenderPass() = default;

		virtual void Init() override;
		virtual void Execute(RenderContext& context) override;
		virtual void OnViewportResize(uint32_t width, uint32_t height) override;
		virtual void Shutdown() override;

	private:
		void SortParticlesByCameraDistance(const RenderContext& context, std::vector<Particle>& particlesToSort);
		void PackDataForGPU(const RenderContext& context, const std::vector<Particle>& sortedParticles);
		void BindTextures(const RenderContext& context, uint32_t usedTextureSlots);
		void DrawParticles();

		// One draw call per blend mode, since a single instanced draw cannot switch GL blend state mid-flight
		void DrawBatch(const RenderContext& context, const std::vector<Particle>& particles, RendererAPI::BlendMode mode);

	private:
		SharedPtr<VertexArray> m_ParticleVAO;
		SharedPtr<VertexBuffer> m_ParticleVBO;
		SharedPtr<Shader> m_ParticleShader;

		std::vector<Particle> m_AlphaParticles;
		std::vector<Particle> m_AdditiveParticles;

		std::vector<ParticleVertex> m_InstancedData;
		std::array<UUID, 32> m_TextureSlots; // Max of 32 texture slots in the shader (including the default white texture at slot 0)
		uint8_t m_CurrentTexSlotIndex = 1;
	};

}