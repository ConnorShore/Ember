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

		// Direction ShadowMap Buffer
		{
			Ember::FramebufferSpecification specs;
			specs.Width = 2048;
			specs.Height = 2048;
			specs.AttachmentSpecs = {
				Ember::FramebufferTextureFormat::DEPTH24STENCIL8
			};
			m_DirectionalShadowMapBuffer = Framebuffer::Create(specs);
		}
		// Spot ShadowMap Buffer
		{
			Ember::FramebufferSpecification specs;
			specs.Width = 2048;
			specs.Height = 2048;
			specs.AttachmentSpecs = {
				Ember::FramebufferTextureFormat::DEPTH24STENCIL8
			};
			m_SpotShadowMapBuffer = Framebuffer::Create(specs);
		}
		// HDR Framebuffer
		{
			Ember::FramebufferSpecification specs;
			specs.Width = 1;
			specs.Height = 1;
			specs.AttachmentSpecs = {
				Ember::FramebufferTextureFormat::RGBA16F,
				Ember::FramebufferTextureFormat::RGBA16F,
				Ember::FramebufferTextureFormat::DEPTH24STENCIL8
			};
			m_HdrFramebuffer = Framebuffer::Create(specs);
		}

		// Bloom (Ping Pong) Framebuffers
		{

			Ember::FramebufferSpecification specs;
			specs.Width = 1;
			specs.Height = 1;
			specs.AttachmentSpecs = {
				Ember::FramebufferTextureFormat::RGBA16F,
				Ember::FramebufferTextureFormat::RGBA16F
			};
			for (unsigned int i = 0; i < m_PingPongBuffers.size(); i++)
				m_PingPongBuffers[i] = Framebuffer::Create(specs);
		}

		m_CameraUniformBuffer = UniformBuffer::Create(sizeof(Matrix4f), 0);
		m_ShadowUniformBuffer = UniformBuffer::Create(sizeof(Matrix4f) * 2, 1);

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
		CreateDirectionalShadowMap(registry);
		CreateSpotlightShadowMap(registry);

		RenderDeferredGeometry(registry);
		RenderDeferredLighting(registry);

		// The Forward Pipeline
		RenderForwardEntities(registry);
		RenderTransparentEntities(registry);


		RenderAction::UseDepthTest(false);
		auto& assetManager = Application::Instance().GetAssetManager();
		auto blurShader = assetManager.GetAsset<Shader>(Constants::Assets::GaussianBlurShad);

		// Bloom post-processing (horizontal and vertical blur passes)
		bool horizontalPass = true, firstIter = true;
		int amount = 10; // Push this back up to 10 so it blurs nicely!
		blurShader->Bind();
		blurShader->SetInt("u_Image", 0);
		for (unsigned int i = 0; i < amount; i++)
		{
			m_PingPongBuffers[horizontalPass]->Bind();
			blurShader->SetInt("u_HorizontalPass", horizontalPass);

			if (firstIter)
				RenderAction::SetTextureUnit(0, m_HdrFramebuffer->GetColorAttachmentID(1));
			else
				RenderAction::SetTextureUnit(0, m_PingPongBuffers[!horizontalPass]->GetColorAttachmentID(0));

			Renderer3D::Submit(m_ScreenQuad->GetVertexArray());
			horizontalPass = !horizontalPass;
			if (firstIter)
				firstIter = false;
		}

		// Apply the bloom effect by blending the blurred bright areas back onto the scene
		RenderAction::SetFramebuffer(m_RenderSceneState.OutputFramebufferId);
		RenderAction::SetViewport(m_RenderSceneState.ViewportDimensions);
		RenderAction::Clear(Ember::RendererAPI::RenderBit::Color | Ember::RendererAPI::RenderBit::Depth);
		RenderAction::UseDepthTest(false);

		auto bloomShader = assetManager.GetAsset<Shader>(Constants::Assets::BloomShad);
		bloomShader->Bind();
		bloomShader->SetFloat("u_Exposure", 1.0f);

		bloomShader->SetInt("u_Scene", 0);     
		bloomShader->SetInt("u_BloomBlur", 1); 

		RenderAction::SetTextureUnit(0, m_HdrFramebuffer->GetColorAttachmentID(0));
		RenderAction::SetTextureUnit(1, m_PingPongBuffers[!horizontalPass]->GetColorAttachmentID(0));

		Renderer3D::Submit(m_ScreenQuad->GetVertexArray());

		// Overlays
		Render2DEntities(registry);

		ResetRenderState();
	}

	void RenderSystem::OnViewportResize(unsigned int width, unsigned int height)
	{
		if (m_GBuffer)
		{
			m_GBuffer->ViewportResize(width, height);
			m_HdrFramebuffer->ViewportResize(width, height);
			m_PingPongBuffers[0]->ViewportResize(width, height);
			m_PingPongBuffers[1]->ViewportResize(width, height);
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

	void RenderSystem::CreateShadowMaps(Registry* registry)
	{
		CreateDirectionalShadowMap(registry);
		CreateSpotlightShadowMap(registry);
	}

	void RenderSystem::CreateDirectionalShadowMap(Registry* registry)
	{
		// Get directional light view matrix to create shadow map
		View lightView = registry->Query<DirectionalLightComponent, TransformComponent>();
		unsigned int index = 0;
		for (EntityID entity : lightView)
		{
			if (index >= Constants::Renderer::MaxDirectionalLights)
				break;

			auto [light, transform] = registry->GetComponents<DirectionalLightComponent, TransformComponent>(entity);

			// TODO: These props are just hard coded but will eventually move to "Dynamic Shadow Frustums" and "Cascaded Shadow Maps"
			Matrix4f lightProjection = Math::Orthographic(-35.0f, 35.0f, -35.0f, 35.0f, 1.0f, 500.0f);
			Vector3f target = Vector3f(0.0f, 0.0f, 0.0f);
			Vector3f eye = target - (Math::Normalize(light.Direction) * 40.0f); // Pull back 40 units
			Vector3f up = Vector3f(0.0f, 1.0f, 0.0f);
			if (std::abs(light.Direction.y) > 0.99f)
				up = Vector3f(0.0f, 0.0f, 1.0f);
			Matrix4f lightView = Math::LookAt(eye, target, up);
			m_RenderSceneState.DirectionalLightViewMatrix = lightProjection * lightView;

			// Set uniform buffer for directional light (offset 0)
			m_ShadowUniformBuffer->SetData(&m_RenderSceneState.DirectionalLightViewMatrix, sizeof(Matrix4f), 0);

			index++;
		}
		RenderGeometryForShadowMaps(registry, m_RenderSceneState.DirectionalLightViewMatrix, m_DirectionalShadowMapBuffer);
	}

	void RenderSystem::CreateSpotlightShadowMap(Registry* registry)
	{
		// Get spotlight view matrix to create shadow map
		View lightView = registry->Query<SpotLightComponent, TransformComponent>();
		unsigned int index = 0;
		for (EntityID entity : lightView)
		{
			// TODO: Will create a 4-layer texture array for spotlight shadow maps to hold multiple shadow maps in the future, 
			// but for now we will just use one shadow map and overwrite it for each spotlight. This means only one spotlight can cast shadows at a time
			if (index >= Constants::Renderer::MaxSpotLights)
				break;

			auto [light, transform] = registry->GetComponents<SpotLightComponent, TransformComponent>(entity);

			// TODO: These props are just hard coded but will eventually move to "Dynamic Shadow Frustums" and "Cascaded Shadow Maps"
			Matrix4f lightProjection = Math::Perspective(Math::Degrees(light.OuterCutOffAngle) * 2.0f, 1.0f, 1.0f, 100.0f);
			Vector3f target = light.Direction + transform.Position;	// Look in the direction of the spotlight
			Vector3f eye = transform.Position;
			Vector3f up = Vector3f(0.0f, 1.0f, 0.0f);
			Matrix4f lightView = Math::LookAt(eye, target, up);
			m_RenderSceneState.SpotLightViewMatrix = lightProjection * lightView;

			// Set uniform buffer for spotlight (offset -> 1 mat4)
			m_ShadowUniformBuffer->SetData(&m_RenderSceneState.SpotLightViewMatrix, sizeof(Matrix4f), sizeof(Matrix4f));

			index++;
		}

		RenderGeometryForShadowMaps(registry, m_RenderSceneState.SpotLightViewMatrix, m_SpotShadowMapBuffer);
	}

	void RenderSystem::RenderGeometryForShadowMaps(Registry* registry, const Matrix4f& lightViewMatrix, const SharedPtr<Framebuffer>& shadowMapBuffer)
	{
		auto& assetManager = Application::Instance().GetAssetManager();
		auto shadowShader = assetManager.GetAsset<Shader>(Constants::Assets::StandardShadowShad);

		shadowMapBuffer->Bind();

		RenderAction::SetViewport(0, 0, shadowMapBuffer->GetSpecification().Width, shadowMapBuffer->GetSpecification().Height);
		RenderAction::Clear(Ember::RendererAPI::RenderBit::Depth);
		RenderAction::UseDepthTest(true);

		Renderer3D::BeginFrame();

		shadowShader->Bind();
		shadowShader->SetMatrix4(Constants::Uniforms::LightViewMatrix, lightViewMatrix);

		for (EntityID entity : m_RenderQueueBuckets.Opaque)
		{
			auto [mesh, material, transform] = registry->GetComponents<MeshComponent, MaterialComponent, TransformComponent>(entity);
			shadowShader->SetMatrix4(Constants::Uniforms::Transform, transform.WorldTransform);
			Renderer3D::Submit(mesh.Mesh->GetVertexArray());
		}

		Renderer3D::EndFrame();
	}

	void RenderSystem::RenderDeferredGeometry(Registry* registry)
	{
		m_GBuffer->Bind();
		RenderAction::SetViewport(0, 0, m_GBuffer->GetSpecification().Width, m_GBuffer->GetSpecification().Height);

		RenderAction::SetClearColor(Ember::Vector4f(0.0f, 0.0f, 0.0f, 1.0f));
		RenderAction::Clear(Ember::RendererAPI::RenderBit::Color | Ember::RendererAPI::RenderBit::Depth);
		RenderAction::UseDepthTest(true);

		// Bind default white as the default texture for all units to avoid accidentally sampling from unbound texture units in the shader
		auto defaultWhite = Application::Instance().GetAssetManager().GetAsset<Texture>(Constants::Assets::DefaultWhiteTex);
		auto defaultNormal = Application::Instance().GetAssetManager().GetAsset<Texture>(Constants::Assets::DefaultNormalTex);
		RenderAction::SetTextureUnit(0, defaultWhite->GetID());
		RenderAction::SetTextureUnit(1, defaultNormal->GetID());
		RenderAction::SetTextureUnit(2, defaultWhite->GetID());

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
		//RenderAction::SetFramebuffer(m_RenderSceneState.OutputFramebufferId);
		m_HdrFramebuffer->Bind();
		RenderAction::SetViewport(m_RenderSceneState.ViewportDimensions);

		RenderAction::SetClearColor(Ember::Vector4f(0.0f, 0.0f, 0.0f, 1.0f));
		RenderAction::Clear(Ember::RendererAPI::RenderBit::Color);

		auto& assetManager = Application::Instance().GetAssetManager();
		auto litShader = assetManager.GetAsset<Shader>(Constants::Assets::StandardLitShad);

		litShader->Bind();
		litShader->SetFloat3(Constants::Uniforms::CameraPosition, m_RenderSceneState.CameraTransform[3]);
		litShader->SetInt("gAlbedoRoughness", 0);
		litShader->SetInt("gNormalMetallic", 1);
		litShader->SetInt("gPositionAO", 2);
		litShader->SetInt("directionShadowMap", 3);
		litShader->SetInt("spotShadowMap", 4);

		RenderAction::SetTextureUnit(0, m_GBuffer->GetColorAttachmentID(0));
		RenderAction::SetTextureUnit(1, m_GBuffer->GetColorAttachmentID(1));
		RenderAction::SetTextureUnit(2, m_GBuffer->GetColorAttachmentID(2));
		RenderAction::SetTextureUnit(3, m_DirectionalShadowMapBuffer->GetDepthAttachmentID());
		RenderAction::SetTextureUnit(4, m_SpotShadowMapBuffer->GetDepthAttachmentID());

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

		// Set spotlights
		{
			View lightView = registry->Query<SpotLightComponent, TransformComponent>();
			unsigned int index = 0;
			for (EntityID entity : lightView)
			{
				if (index >= Constants::Renderer::MaxSpotLights)
					break;

				auto [light, transform] = registry->GetComponents<SpotLightComponent, TransformComponent>(entity);
				litShader->SetFloat3(std::format("u_SpotLights[{}].Position", index), transform.Position);
				litShader->SetFloat3(std::format("u_SpotLights[{}].Direction", index), light.Direction);
				litShader->SetFloat3(std::format("u_SpotLights[{}].Color", index), light.Color);
				litShader->SetFloat(std::format("u_SpotLights[{}].Intensity", index), light.Intensity);
				litShader->SetFloat(std::format("u_SpotLights[{}].CutOff", index), light.CutOff);
				litShader->SetFloat(std::format("u_SpotLights[{}].OuterCutOff", index), light.OuterCutOff);

				index++;
			}

			litShader->SetInt(Constants::Uniforms::ActiveSpotLights, index);
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
		RenderAction::CopyDepthBuffer(m_GBuffer->GetID(), m_HdrFramebuffer->GetID(), m_RenderSceneState.ViewportDimensions);
		m_HdrFramebuffer->Bind();
		//RenderAction::CopyDepthBuffer(m_GBuffer->GetID(), m_RenderSceneState.OutputFramebufferId, m_RenderSceneState.ViewportDimensions);
		//RenderAction::SetFramebuffer(m_RenderSceneState.OutputFramebufferId);

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