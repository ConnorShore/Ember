#pragma once

#include "RenderPass.h"

#include <vector>
#include <array>

namespace Ember {

	class ParticleManager;
	struct Particle;

	class ParticleRenderPass : public RenderPass
	{
	public:
		ParticleRenderPass() = default;
		virtual ~ParticleRenderPass() = default;

		virtual void Init() override;
		virtual void Execute(RenderContext& context) override;
		virtual void Shutdown() override;

	private:
		void SortParticlesByCameraDistance(const RenderContext& context, std::vector<Particle>& particlesToSort);
		uint32_t PackDataForGPU(const RenderContext& context, const std::vector<Particle>& sortedParticles);
		void BindTextures(const RenderContext& context, uint32_t usedTextureSlots);
		void DrawParticles();

	private:
		ParticleManager& m_ParticleManager;
		
		SharedPtr<VertexArray> m_ParticleVAO;
		SharedPtr<VertexBuffer> m_ParticleVBO;
		SharedPtr<Shader> m_ParticleShader;

		std::vector<ParticleVertex> m_InstancedData;
		std::array<UUID, 32> m_TextureSlots; // Max of 32 texture slots in the shader (including the default white texture at slot 0)
	};

}