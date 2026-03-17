#include "ebpch.h"
#include "RenderSystem.h"

#include "Ember/Core/Application.h"
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

		// GBuffer
		{
			Ember::FramebufferSpecification specs;
			specs.Width = 1;
			specs.Height = 1;
			specs.AttachmentSpecs = {
				Ember::FramebufferTextureFormat::RGBA8,
				Ember::FramebufferTextureFormat::RGBA16F,
				Ember::FramebufferTextureFormat::RGBA16F,
				Ember::FramebufferTextureFormat::DEPTH24STENCIL8
			};
			m_GBuffer = Framebuffer::Create(specs);
		}

		// ShadowMap Buffer
		{
			Ember::FramebufferSpecification specs;
			specs.Width = 2048;
			specs.Height = 2048;
			specs.AttachmentSpecs = {
				Ember::FramebufferTextureFormat::DEPTH24STENCIL8
			};
			m_ShadowMapBuffer = Framebuffer::Create(specs);
		}

		m_CameraUniformBuffer = UniformBuffer::Create(sizeof(Matrix4f), 0);

		m_ScreenQuad = PrimitiveGenerator::CreateQuad(2.0f, 2.0f);

		m_RenderSceneState.Reset();

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
		InitializeRenderState();

		// Save output framebuffer
		RenderAction::GetPreviousFramebuffer(&m_RenderSceneState.OutputFramebufferId);

		SetSceneCamera(registry);

		if (!m_RenderSceneState.IsCameraFound) return;

		// Sort entities into render queue buckets
		SortEntitiesByRenderQueue(registry);

		// The Deferred Pipeline
		CreateShadowMap(registry);
		RenderDeferredGeometry(registry);
		RenderDeferredLighting(registry);

		// The Forward Pipeline
		RenderForwardEntities(registry);
		RenderTransparentEntities(registry);

		// Overlays
		Render2DEntities(registry);

		ResetRenderState();
	}


	void RenderSystem::OnViewportResize(unsigned int width, unsigned int height)
	{
		if (m_GBuffer)
		{
			m_GBuffer->ViewportResize(width, height);
		}
	}

	void RenderSystem::InitializeRenderState()
	{
		RenderAction::SetClearColor(Ember::Vector4f(0.0f, 0.0f, 0.0f, 1.0));
		RenderAction::Clear();

		m_RenderQueueBuckets.Clear();
		m_RenderSceneState.Reset();
	}

	void RenderSystem::SetSceneCamera(Registry* registry)
	{
		View cameraView = registry->Query<CameraComponent, TransformComponent>();
		for (EntityID cameraEntity : cameraView)
		{
			auto [camera, transform] = registry->GetComponents<CameraComponent, TransformComponent>(cameraEntity);
			if (camera.IsActive)
			{
				m_RenderSceneState.ActiveCamera = camera;
				m_RenderSceneState.CameraTransform = Math::Translate(transform.Position) * Math::GetRotationMatrix(transform.Rotation);
				m_RenderSceneState.IsCameraFound = true;

				// set uniform buffer
				Matrix4f viewProjectionMat = camera.Camera.GetProjectionMatrix() * Math::Inverse(m_RenderSceneState.CameraTransform);
				m_CameraUniformBuffer->SetData(&viewProjectionMat, sizeof(Matrix4f));

				break;
			}
		}
	}

	void RenderSystem::CreateShadowMap(Registry* registry)
	{
		// Get directional light view matrix to create shadow map
		View lightView = registry->Query<DirectionalLightComponent, TransformComponent>();
		unsigned int index = 0;
		for (EntityID entity : lightView)
		{
			if (index >= Constants::Renderer::MaxDirectionalLights)
				break;

			auto [light, transform] = registry->GetComponents<DirectionalLightComponent, TransformComponent>(entity);

			// TODO: These props are just hardcoded but will eventually move to "Dynamic Shadow Frustums" and "Cascaded Shadow Maps"
			Matrix4f lightProjection = Math::Orthographic(-35.0f, 35.0f, -35.0f, 35.0f, 1.0f, 100.0f);
			Vector3f target = Vector3f(0.0f, 0.0f, 0.0f);
			Vector3f eye = target - (Math::Normalize(light.Direction) * 40.0f); // Pull back 40 units
			Matrix4f lightView = Math::LookAt(eye, target, Vector3f(0.0f, 1.0f, 0.0f));
			m_RenderSceneState.LightViewMatrix = lightProjection * lightView;

			index++;
		}

		auto& assetManager = Application::Instance().GetAssetManager();
		auto shadowShader = assetManager.GetAsset<Shader>(Constants::Assets::StandardShadow);

		m_ShadowMapBuffer->Bind();
		RenderAction::SetViewport(0, 0, m_ShadowMapBuffer->GetSpecification().Width, m_ShadowMapBuffer->GetSpecification().Height);
		RenderAction::Clear(Ember::RendererAPI::RenderBit::Depth);
		RenderAction::UseDepthTest(true);
		RenderAction::UseFaceCulling(true);
		RenderAction::CullFace(RendererAPI::Face::Front);

		Renderer3D::BeginFrame();

		shadowShader->Bind();
		shadowShader->SetMatrix4(Constants::Uniforms::LightViewMatrix, m_RenderSceneState.LightViewMatrix);	// TODO: Move to UniformBuffer

		for (EntityID entity : m_RenderQueueBuckets.Opaque)
		{
			auto [mesh, material, transform] = registry->GetComponents<MeshComponent, MaterialComponent, TransformComponent>(entity);
			shadowShader->SetMatrix4(Constants::Uniforms::Transform, transform.WorldTransform);
			Renderer3D::Submit(mesh.Mesh->GetVertexArray());
		}

		Renderer3D::EndFrame();

		RenderAction::CullFace(RendererAPI::Face::Back);
	}

	void RenderSystem::RenderDeferredGeometry(Registry* registry)
	{

		m_GBuffer->Bind();
		RenderAction::SetViewport(0, 0, m_GBuffer->GetSpecification().Width, m_GBuffer->GetSpecification().Height);

		RenderAction::SetClearColor(Ember::Vector4f(0.0f, 0.0f, 0.0f, 1.0f));
		RenderAction::Clear(Ember::RendererAPI::RenderBit::Color | Ember::RendererAPI::RenderBit::Depth);
		RenderAction::UseDepthTest(true);

		Renderer3D::BeginFrame();

		for (EntityID entity : m_RenderQueueBuckets.Opaque)
		{
			auto [mesh, material, transform] = registry->GetComponents<MeshComponent, MaterialComponent, TransformComponent>(entity);
			Renderer3D::Submit(mesh.Mesh->GetVertexArray(), material, transform.WorldTransform);
		}

		Renderer3D::EndFrame();
	}

	void RenderSystem::RenderDeferredLighting(Registry* registry)
	{
		int dims[4] = { 0 };
		RenderAction::GetViewportDimensions(dims);
		m_RenderSceneState.ViewportDimensions = Vector4<int>(dims[0], dims[1], dims[2], dims[3]);

		RenderAction::UseDepthTest(false);
		RenderAction::UseFaceCulling(false);
		RenderAction::SetFramebuffer(m_RenderSceneState.OutputFramebufferId);
		RenderAction::SetViewport(m_RenderSceneState.ViewportDimensions);

		auto& assetManager = Application::Instance().GetAssetManager();
		auto litShader = assetManager.GetAsset<Shader>(Constants::Assets::StandardLitShad);

		litShader->Bind();
		litShader->SetFloat3(Constants::Uniforms::CameraPosition, m_RenderSceneState.CameraTransform[3]);
		litShader->SetMatrix4(Constants::Uniforms::LightViewMatrix, m_RenderSceneState.LightViewMatrix);	// TODO: Move to UniformBuffer
		RenderAction::SetTextureUnit(0, m_GBuffer->GetColorAttachmentID(0));
		RenderAction::SetTextureUnit(1, m_GBuffer->GetColorAttachmentID(1));
		RenderAction::SetTextureUnit(2, m_GBuffer->GetColorAttachmentID(2));
		RenderAction::SetTextureUnit(3, m_ShadowMapBuffer->GetDepthAttachmentID());

		// Set Directional Light
		{
			View lightView = registry->Query<DirectionalLightComponent, TransformComponent>();
			unsigned int index = 0;
			for (EntityID entity : lightView)
			{
				if (index >= Constants::Renderer::MaxDirectionalLights)
					break;

				auto [light, transform] = registry->GetComponents<DirectionalLightComponent, TransformComponent>(entity);
				litShader->SetFloat3(std::format("u_DirectionalLights[{}].Direction", index), light.Direction);
				litShader->SetFloat3(std::format("u_DirectionalLights[{}].Color", index), light.Color);
				litShader->SetFloat(std::format("u_DirectionalLights[{}].Intensity", index), light.Intensity);

				index++;
			}

			litShader->SetInt(Constants::Uniforms::ActiveDirectionalLights, index);
		}

		// Set Point Lights
		{
			View lightView = registry->Query<PointLightComponent, TransformComponent>();
			unsigned int index = 0;
			for (EntityID entity : lightView)
			{
				if (index >= Constants::Renderer::MaxPointLights)
					break;

				auto [light, transform] = registry->GetComponents<PointLightComponent, TransformComponent>(entity);
				litShader->SetFloat3(std::format("u_PointLights[{}].Position", index), transform.Position);
				litShader->SetFloat3(std::format("u_PointLights[{}].Color", index), light.Color);
				litShader->SetFloat(std::format("u_PointLights[{}].Intensity", index), light.Intensity);

				index++;
			}

			litShader->SetInt(Constants::Uniforms::ActivePointLights, index);
		}

		Renderer3D::Submit(m_ScreenQuad->GetVertexArray());
	}

	void RenderSystem::RenderForwardEntities(Registry* registry)
	{
		// Copy depth buffer for forward rendering
		RenderAction::CopyDepthBuffer(m_GBuffer->GetID(), m_RenderSceneState.OutputFramebufferId, m_RenderSceneState.ViewportDimensions);

		RenderAction::SetFramebuffer(m_RenderSceneState.OutputFramebufferId);
		RenderAction::UseDepthTest(true);

		Renderer3D::BeginFrame();

		for (EntityID entity : m_RenderQueueBuckets.Forward)
		{
			auto [mesh, material, transform] = registry->GetComponents<MeshComponent, MaterialComponent, TransformComponent>(entity);
			Renderer3D::Submit(mesh.Mesh->GetVertexArray(), material, transform.WorldTransform);
		}

		Renderer3D::EndFrame();
	}

	void RenderSystem::RenderTransparentEntities(Registry* registry)
	{

	}

	void RenderSystem::Render2DEntities(Registry* registry)
	{
		RenderAction::UseDepthTest(false);
		
		Renderer2D::BeginFrame();

		View view = registry->Query<SpriteComponent, TransformComponent>();
		for (EntityID entity : view)
		{
			auto [sprite, transform] = registry->GetComponents<SpriteComponent, TransformComponent>(entity);
			if (sprite.Texture == nullptr)
				Renderer2D::DrawQuad(Vector2f(transform.Position.x, transform.Position.y),
					Vector2f(transform.Scale.x, transform.Scale.y), sprite.Color);
			else
				Renderer2D::DrawQuad(Vector2f(transform.Position.x, transform.Position.y),
					Vector2f(transform.Scale.x, transform.Scale.y), sprite.Color, sprite.Texture);
		}

		Renderer2D::EndFrame();
	}

	void RenderSystem::ResetRenderState()
	{
		RenderAction::UseDepthTest(true);
	}

	void RenderSystem::SortEntitiesByRenderQueue(Registry* registry)
	{
		View view = registry->Query<MeshComponent, MaterialComponent, TransformComponent>();
		for (EntityID entity : view)
		{
			auto [mesh, material, transform] = registry->GetComponents<MeshComponent, MaterialComponent, TransformComponent>(entity);
			switch (material.Material->GetRenderQueue())
			{
			case RenderQueue::Opaque:
				m_RenderQueueBuckets.Opaque.push_back(entity);
				break;
			case RenderQueue::Forward:
				m_RenderQueueBuckets.Forward.push_back(entity);
				break;
			case RenderQueue::Transparent:
				m_RenderQueueBuckets.Transparent.push_back(entity);
				break;
			default:
				EB_CORE_ASSERT(false, "Unknown Render Queue type!");
				break;
			}
		}
	}

}