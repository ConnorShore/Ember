#include "ebpch.h"
#include "ParticleRenderPass.h"

#include "Ember/Scene/Scene.h"
#include "Ember/Math/Math.h"

#include "Ember/ECS/System/ParticleSystem.h"

#include "Ember/Render/Framebuffer.h"
#include "Ember/Render/RenderAction.h"
#include "Ember/Render/Renderer3D.h"
#include "Ember/Render/PrimitiveGenerator.h"

namespace Ember {

	void ParticleRenderPass::Init()
	{
		m_ParticleManager = Application::Instance().GetSystem<ParticleSystem>()->GetParticleManager();
		
		auto quadVAO = PrimitiveGenerator::CreateQuad(1.0f, 1.0f)->GetVertexArray();
		auto quadVBO = quadVAO->GetVertexBuffer();
		auto quadIBO = quadVAO->GetIndexBuffer();
		m_ParticleVBO = VertexBuffer::Create(Constants::Renderer::MaxParticles * sizeof(ParticleVertex));
		m_ParticleVBO->SetLayout({
			{ ShaderDataType::Float3, "i_Position", true /* Instanced */ },
			{ ShaderDataType::Float, "i_Rotation", true /* Instanced */ },
			{ ShaderDataType::Float2, "i_Scale", true  /* Instanced */ },
			{ ShaderDataType::Float4, "i_Color", true  /* Instanced */ },
			{ ShaderDataType::UInt, "i_TexIndex", true /* Instanced */}
			});

		m_ParticleVAO = VertexArray::Create();
		m_ParticleVAO->AddVertexBuffer(quadVBO);
		m_ParticleVAO->AddVertexBuffer(m_ParticleVBO);
		m_ParticleVAO->SetIndexBuffer(quadIBO);

		m_TextureSlots.fill(Constants::InvalidUUID); // Mark all texture slots as empty
		m_TextureSlots[0] = Constants::Assets::DefaultWhiteTexUUID; // Slot 0 is reserved for the default white texture
	}

	void ParticleRenderPass::Execute(RenderContext& context)
	{
		m_InstancedData.clear();

		RenderAction::UseBlending(true);
		RenderAction::UseDepthTest(true);
		RenderAction::UseDepthMask(false);

		// Create a copy of the particle list so we can sort without modifying the original order in the manager
		auto pool = m_ParticleManager.GetParticles();

		SortParticlesByCameraDistance(context, pool);
		uint32_t usedTextureSlots = PackDataForGPU(context, pool);

		if (m_InstancedData.empty())
			return;

		BindTextures(context, usedTextureSlots);
		DrawParticles();

		m_FramebufferInputs["HDRScene"]->Unbind();

		RenderAction::UseDepthMask(true);
		RenderAction::UseBlending(false);
	}

	void ParticleRenderPass::Shutdown()
	{
	}

	void ParticleRenderPass::SortParticlesByCameraDistance(const RenderContext& context, std::vector<Particle>& particlesToSort)
	{
		Vector3f camPos = Vector3f(context.CameraTransform[3]);
		std::sort(particlesToSort.begin(), particlesToSort.end(), [&camPos](const Particle& a, const Particle& b) {
			return Math::Distance(a.Position, camPos) > Math::Distance(b.Position, camPos); // Back to Front
		});
	}

	// Returns the number of texture slots used (including the default white texture at slot 0)
	uint32_t ParticleRenderPass::PackDataForGPU(const RenderContext& context, const std::vector<Particle>& sortedParticles)
	{
		m_InstancedData.reserve(sortedParticles.size());

		// Array to track which textures we have bound this frame
		uint32_t usedTextureSlots = 1; // Start at 1 since 0 is reserved for the default white texture
		for (const auto& particle : sortedParticles)
		{
			if (!particle.Active)
				continue;

			ParticleVertex data;
			data.Position = particle.Position;
			data.Color = particle.CurrentColor;

			float speed = Math::Length(particle.Velocity);

			// Calculate the rotation angle based on the velocity vector
			float velocityAngle = std::atan2(particle.Velocity.y, particle.Velocity.x);
			if (particle.AlignWithVelocity)
			{
				float speed = Math::Length(particle.Velocity);
				data.Scale = {
					particle.CurrentScale + (speed * particle.StretchFactor), // X stretches!
					particle.CurrentScale                                     // Y stays normal
				};

				// Rotate in direction of velocity vector
				data.Rotation = std::atan2(particle.Velocity.y, particle.Velocity.x);
			}
			else
			{
				data.Scale = { particle.CurrentScale, particle.CurrentScale };
				data.Rotation = particle.Rotation;
			}

			// Find or assign a texture slot
			uint32_t textureIndex = 0; // Default to white texture
			if (particle.TextureHandle != Constants::InvalidUUID)
			{
				auto it = std::find(m_TextureSlots.begin(), m_TextureSlots.end(), particle.TextureHandle);
				if (it != m_TextureSlots.end())
				{
					// Removed the +1 since we assing the default white texture to slot 0 in Init(), so the first texture from particles will be at slot 1, which matches the index in m_TextureSlots
					textureIndex = (uint32_t)std::distance(m_TextureSlots.begin(), it);
					//textureIndex = (uint32_t)std::distance(m_TextureSlots.begin(), it) + 1; // +1 because 0 is our default white texture
				}
				else
				{
					// New texture found! Add it to the list.
					m_TextureSlots[usedTextureSlots++] = particle.TextureHandle;
					textureIndex = usedTextureSlots;
				}
			}

			data.TexIndex = textureIndex;
			m_InstancedData.push_back(data);
		}

		if (m_InstancedData.empty())
			return 0;

		m_ParticleVBO->SetData(m_InstancedData.data(), (uint32_t)(m_InstancedData.size() * sizeof(ParticleVertex)));
		return usedTextureSlots;
	}

	void ParticleRenderPass::BindTextures(const RenderContext& context, uint32_t usedTextureSlots)
	{
		m_FramebufferInputs["HDRScene"]->Bind();
		auto particleShad = Application::Instance().GetAssetManager().GetAsset<Shader>(Constants::Assets::ParticleShad);
		particleShad->Bind();

		// Bind Default White Texture to Slot 0
		auto defaultWhite = Application::Instance().GetAssetManager().GetAsset<Texture2D>(Constants::Assets::DefaultWhiteTex);
		RenderAction::SetTextureUnit(0, defaultWhite->GetID());

		// Bind the rest of the textures found this frame
		int samplers[32];
		//samplers[0] = 0; // Slot 0 // Removed this since we bind default texture already

		for (uint32_t i = 0; i < usedTextureSlots; i++)
		{
			auto tex = Application::Instance().GetAssetManager().GetAsset<Texture2D>(m_TextureSlots[i]);
			RenderAction::SetTextureUnit(i, tex->GetID());
			samplers[i] = i;
		}

		// Tell the shader about the array of samplers
		particleShad->SetIntArray("u_Textures", samplers, usedTextureSlots);

		// The shader needs the camera up/right vectors to build the billboard matrix!
		Vector3f camRight = Vector3f(context.CameraTransform[0]);
		Vector3f camUp = Vector3f(context.CameraTransform[1]);
		particleShad->SetFloat3(Constants::Uniforms::CameraRight, camRight);
		particleShad->SetFloat3(Constants::Uniforms::CameraUp, camUp);
	}

	void ParticleRenderPass::DrawParticles()
	{
		uint32_t indexCount = m_ParticleVAO->GetIndexBuffer()->GetCount();
		uint32_t instanceCount = (uint32_t)m_InstancedData.size();

		RenderAction::DrawIndexedInstanced(m_ParticleVAO, indexCount, instanceCount);
	}

}