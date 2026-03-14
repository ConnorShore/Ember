#include "ebpch.h"
#include "RenderSystem.h"

#include "Ember/ECS/Component/Components.h"
#include "Ember/Render/RenderAction.h"
#include "Ember/Render/Renderer2D.h"
#include "Ember/Render/Renderer3D.h"
#include "Ember/Render/PrimitiveGenerator.h"

namespace Ember {

	void RenderSystem::OnAttach(Registry* registry)
	{
		Renderer2D::Init();
		Renderer3D::Init();

		m_GeometryShader = Shader::Create("assets/shaders/Geometry.glsl");
		m_LightingShader = Shader::Create("assets/shaders/Lighting.glsl");

		// TODO: See if size should be window size or arbitrary value
		Ember::FramebufferSpecification specs;
		specs.Width = 1280;
		specs.Height = 720;
		specs.AttachmentSpecs = {
			Ember::FramebufferTextureFormat::RGBA8,
			Ember::FramebufferTextureFormat::RGBA16F,
			Ember::FramebufferTextureFormat::RGBA16F,
			Ember::FramebufferTextureFormat::DEPTH24STENCIL8
		};
		m_GBuffer = Framebuffer::Create(specs);

		m_ScreenQuad = PrimitiveGenerator::CreateQuad(2.0f, 2.0f);

		EB_CORE_INFO("RenderSystem is attached!");
	}

	void RenderSystem::OnDetach(Registry* registry)
	{
		Renderer2D::Shutdown();
		Renderer3D::Shutdown();
		EB_CORE_INFO("RenderSystem is detached!");
	}

	void RenderSystem::OnUpdate(TimeStep delta, Registry* registry)
	{
		RenderAction::SetClearColor(Ember::Vector4f(0.0f, 0.0f, 0.0f, 1.0));
		RenderAction::Clear();

		bool cameraFound = false;
		CameraComponent activeCamera;
		Matrix4f cameraTransformMatrix;
		Vector3f cameraPos;
		View cameraView = registry->Query<CameraComponent, TransformComponent>();
		for (EntityID cameraEntity : cameraView)
		{
			auto [camera, transform] = registry->GetComponents<CameraComponent, TransformComponent>(cameraEntity);
			if (camera.IsActive)
			{
				activeCamera = camera;
				cameraPos = transform.Position;
				cameraTransformMatrix = Math::Translate(transform.Position) * Math::GetRotationMatrix(transform.Rotation);
				cameraFound = true;
				break;
			}
		}

		if (!cameraFound)
		{
			EB_CORE_WARN("No camera found! Scene not rendering");
			return;
		}

		// Save output framebuffer
		int outputFramebuffer;
		RenderAction::GetPreviousFramebuffer(&outputFramebuffer);

		// 3D Geometry Render pass
		{
			m_GBuffer->Bind(); 
			RenderAction::SetViewport(0, 0, m_GBuffer->GetSpecification().Width, m_GBuffer->GetSpecification().Height);

			RenderAction::SetClearColor(Ember::Vector4f(0.0f, 0.0f, 0.0f, 1.0f));
			RenderAction::Clear(Ember::RendererAPI::RenderBit::Color | Ember::RendererAPI::RenderBit::Depth);
			RenderAction::UseDepthTest(true);

			Renderer3D::BeginFrame(activeCamera, cameraTransformMatrix);

			// Render meshes
			View view = registry->Query<MeshComponent, MaterialComponent, TransformComponent>();
			for (EntityID entity : view)
			{
				auto [mesh, material, transform] = registry->GetComponents<MeshComponent, MaterialComponent, TransformComponent>(entity);
				Renderer3D::Submit(mesh.Mesh->GetVertexArray(), material, transform.GetTransformationMatrix());
			}

			Renderer3D::EndFrame();
		}

		// 3D Lighting Pass
		{
			RenderAction::UseDepthTest(false); 
			RenderAction::UseFaceCulling(false);
			RenderAction::SetFramebuffer(outputFramebuffer);

			int dims[4] = { 0 };
			RenderAction::GetViewportDimensions(dims);
			RenderAction::SetViewport(dims[0], dims[1], dims[2], dims[3]);
			//RenderAction::SetViewport(0, 0, dims[2], dims[3]);

			m_LightingShader->Bind();
			m_LightingShader->SetFloat3("u_CameraPos", cameraPos);

			RenderAction::SetTextureUnit(0, m_GBuffer->GetColorAttachmentID(0));
			RenderAction::SetTextureUnit(1, m_GBuffer->GetColorAttachmentID(1));
			RenderAction::SetTextureUnit(2, m_GBuffer->GetColorAttachmentID(2));

			View lightView = registry->Query<PointLightComponent, TransformComponent>();
			unsigned int index = 0;
			for (EntityID entity : lightView)
			{
				if (index >= 32)
					break;

				auto [light, transform] = registry->GetComponents<PointLightComponent, TransformComponent>(entity);
				m_LightingShader->SetFloat3(std::format("u_PointLights[{}].Position", index), transform.Position);
				m_LightingShader->SetFloat3(std::format("u_PointLights[{}].Color", index), light.Color);
				m_LightingShader->SetFloat(std::format("u_PointLights[{}].Intensity", index), light.Intensity);

				index++;
			}

			m_LightingShader->SetInt("u_ActiveLights", index);

			Renderer3D::Submit(m_ScreenQuad->GetVertexArray());
		}


		// 2D Render Pass
		{
			RenderAction::UseDepthTest(false);

			Renderer2D::BeginFrame(activeCamera, cameraTransformMatrix);

			View view = registry->Query<SpriteComponent, TransformComponent>();
			for (EntityID entity : view)
			{
				auto [sprite, transform] = registry->GetComponents<SpriteComponent, TransformComponent>(entity);
				if (sprite.Texture == nullptr)
					Renderer2D::DrawQuad(Vector2f(transform.Position.x, transform.Position.y),
						Vector2f(transform.Size.x, transform.Size.y), sprite.Color);
				else
					Renderer2D::DrawQuad(Vector2f(transform.Position.x, transform.Position.y),
					Vector2f(transform.Size.x, transform.Size.y), sprite.Color, sprite.Texture);
			}

			Renderer2D::EndFrame();
		}

		// Reset state
		RenderAction::UseDepthTest(true);
	}

}