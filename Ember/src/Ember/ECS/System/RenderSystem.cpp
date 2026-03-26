#include "ebpch.h"
#include "RenderSystem.h"

#include "Ember/Core/Application.h"
#include "Ember/ECS/Component/Components.h"
#include "Ember/Render/RenderAction.h"
#include "Ember/Render/Renderer2D.h"
#include "Ember/Render/Renderer3D.h"
#include "Ember/Render/PrimitiveGenerator.h"
#include "Ember/Render/VFX/BloomPass.h"
#include "Ember/Render/VFX/OutlinePass.h"
#include "Ember/Scene/Scene.h"

namespace Ember {

	struct DirectionalLightData
	{
		Vector3f Direction; // 12 bytes
		float Intensity;    // 4 bytes

		Vector3f Color;     // 12 bytes
		float _Padding;     // 4 bytes
	};

	struct SpotLightData
	{
		Vector3f Position;  // 12 bytes
		float Intensity;    // 4 bytes

		Vector3f Direction; // 12 bytes
		float CutOff;       // 4 bytes

		Vector3f Color;     // 12 bytes
		float OuterCutOff;  // 4 bytes
	};

	struct PointLightData
	{
		Vector3f Position;  // 12 bytes
		float Intensity;    // 4 bytes

		Vector3f Color;     // 12 bytes
		float Radius;       // 4 bytes
	};

	// Main block matches shader layout
	struct LightDataBlock
	{
		DirectionalLightData DirectionalLights[Constants::Renderer::MaxDirectionalLights];
		SpotLightData SpotLights[Constants::Renderer::MaxSpotLights];
		PointLightData PointLights[Constants::Renderer::MaxPointLights];

		int ActiveDirectionalLights;
		int ActiveSpotLights;
		int ActivePointLights;
		int _Padding; // Pad the final ints to 16 bytes
	};

	void RenderSystem::OnAttach()
	{
		Renderer2D::Init();
		Renderer3D::Init();

		// GBuffer
		{
			Ember::FramebufferSpecification specs;
			specs.Width = 1;
			specs.Height = 1;
			specs.AttachmentSpecs = {
				Ember::FramebufferTextureFormat::RGBA8,				// AlbedoRoughness
				Ember::FramebufferTextureFormat::RGBA16F,			// NormalMetallic
				Ember::FramebufferTextureFormat::RGBA16F,			// PositionAO
				Ember::FramebufferTextureFormat::RGBA16F,			// Emission
				Ember::FramebufferTextureFormat::RED_INTEGER,		// EntityID
				Ember::FramebufferTextureFormat::DEPTH24STENCIL8	// Depth
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
		// Post Process Framebuffers
		{
			Ember::FramebufferSpecification specs;
			specs.Width = 1;
			specs.Height = 1;
			specs.AttachmentSpecs = {
				Ember::FramebufferTextureFormat::RGBA16F,
				Ember::FramebufferTextureFormat::RGBA16F,	// Do I need an extra slot for the new emissions?
				Ember::FramebufferTextureFormat::RED_INTEGER,
				Ember::FramebufferTextureFormat::DEPTH24STENCIL8
			};
			m_HdrSceneBuffer = Framebuffer::Create(specs);
			m_PostProcessBufferA = Framebuffer::Create(specs);
			m_PostProcessBufferB = Framebuffer::Create(specs);
		}

		m_CameraUniformBuffer = UniformBuffer::Create(sizeof(Matrix4f), 0);
		m_ShadowUniformBuffer = UniformBuffer::Create(sizeof(Matrix4f) * 2, 1);
		m_LightUniformBuffer = UniformBuffer::Create(sizeof(LightDataBlock), 2);

		m_ScreenQuad = PrimitiveGenerator::CreateQuad(2.0f, 2.0f);

		//////////////// Init post processing stack ////////////////////////
		m_PostProcessStack.emplace_back(SharedPtr<BloomPass>::Create());

		auto outlinePass = SharedPtr<OutlinePass>::Create();
		outlinePass->SetGBuffer(m_GBuffer);
		m_PostProcessStack.emplace_back(outlinePass);
		////////////////////////////////////////////////////////////////////

		for (auto& pass : m_PostProcessStack)
			pass->Init();

		m_RenderSceneState.Reset();

		EB_CORE_INFO("RenderSystem is attached!");
	}

	void RenderSystem::OnDetach()
	{
		Renderer2D::Shutdown();
		Renderer3D::Shutdown();
		EB_CORE_INFO("RenderSystem is detached!");
	}

	void RenderSystem::ExecuteRenderPipeline(Registry& registry, bool renderInfiniteGrid)
	{
		// Save output framebuffer
		RenderAction::GetPreviousFramebuffer(&m_RenderSceneState.OutputFramebufferId);

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

		// Render Editor Grid
		if (renderInfiniteGrid)
			RenderInfiniteGrid();

		RenderBillboards(registry);

		HandlePostProcessing(registry);

		// Overlays
		Render2DEntities(registry);

		ResetRenderState();
	}

	void RenderSystem::OnUpdate(TimeStep delta, Scene* scene)
	{
		InitializeRenderState();
		SetSceneCamera(scene->GetRegistry());

		if (m_RenderSceneState.IsCameraFound)
		{
			ExecuteRenderPipeline(scene->GetRegistry(), false);
		}
	}

	void RenderSystem::OnUpdate(TimeStep delta, Scene* scene, const Camera& camera, const Matrix4f& cameraTransform)
	{
		InitializeRenderState();

		// Set render scene state for camera info
		m_RenderSceneState.ActiveCamera = camera;
		m_RenderSceneState.CameraTransform = cameraTransform;
		m_RenderSceneState.IsCameraFound = true;

		Matrix4f viewProjectionMat = camera.GetProjectionMatrix() * Math::Inverse(cameraTransform);
		m_CameraUniformBuffer->SetData(&viewProjectionMat, sizeof(Matrix4f));

		// Update the system
		ExecuteRenderPipeline(scene->GetRegistry(), true);
	}

	void RenderSystem::OnViewportResize(unsigned int width, unsigned int height)
	{
		m_GBuffer->ViewportResize(width, height);
		m_HdrSceneBuffer->ViewportResize(width, height);
		m_PostProcessBufferA->ViewportResize(width, height);
		m_PostProcessBufferB->ViewportResize(width, height);

		for (auto& pass : m_PostProcessStack)
			pass->OnViewportResize(width, height);
	}

	EntityID RenderSystem::GetEntityIDAtPixel(unsigned int x, unsigned int y)
	{
		// Check the Forward buffer first (since it is drawn on top of the world)
		m_HdrSceneBuffer->Bind();
		int forwardPixelData = m_HdrSceneBuffer->ReadPixel(2, x, y);
		m_HdrSceneBuffer->Unbind();

		if (forwardPixelData != Constants::Entities::InvalidEntityID)
		{
			return (EntityID)forwardPixelData;
		}

		// If the Forward buffer was empty, fallback to the Opaque G-Buffer!
		m_GBuffer->Bind();
		int opaquePixelData = m_GBuffer->ReadPixel(4, x, y);
		m_GBuffer->Unbind();

		return (EntityID)opaquePixelData;
	}

	void RenderSystem::InitializeRenderState()
	{
		RenderAction::SetClearColor(Ember::Vector4f(0.0f, 0.0f, 0.0f, 1.0));
		RenderAction::Clear();

		m_RenderQueueBuckets.Clear();
		m_RenderSceneState.Reset();
	}

	void RenderSystem::SetSceneCamera(Registry& registry)
	{
		View cameraView = registry.Query<CameraComponent, TransformComponent>();
		for (EntityID cameraEntity : cameraView)
		{
			auto [camera, transform] = registry.GetComponents<CameraComponent, TransformComponent>(cameraEntity);
			if (camera.IsActive)
			{
				m_RenderSceneState.ActiveCamera = camera.Camera;
				m_RenderSceneState.CameraTransform = Math::Translate(transform.Position) * Math::GetRotationMatrix(transform.Rotation);
				m_RenderSceneState.IsCameraFound = true;

				// set uniform buffer
				Matrix4f viewProjectionMat = camera.Camera.GetProjectionMatrix() * Math::Inverse(m_RenderSceneState.CameraTransform);
				m_CameraUniformBuffer->SetData(&viewProjectionMat, sizeof(Matrix4f));

				break;
			}
		}
	}

	void RenderSystem::CreateShadowMaps(Registry& registry)
	{
		CreateDirectionalShadowMap(registry);
		CreateSpotlightShadowMap(registry);
	}

	void RenderSystem::CreateDirectionalShadowMap(Registry& registry)
	{
		// Get directional light view matrix to create shadow map
		View lightView = registry.Query<DirectionalLightComponent, TransformComponent>();
		unsigned int index = 0;
		for (EntityID entity : lightView)
		{
			if (index >= Constants::Renderer::MaxDirectionalLights)
				break;

			auto [light, transform] = registry.GetComponents<DirectionalLightComponent, TransformComponent>(entity);
			Vector3f lightDirection = transform.GetForward();

			// TODO: These props are just hard coded but will eventually move to "Dynamic Shadow Frustums" and "Cascaded Shadow Maps"
			Matrix4f lightProjection = Math::Orthographic(-35.0f, 35.0f, -35.0f, 35.0f, 1.0f, 500.0f);
			Vector3f target = Vector3f(0.0f, 0.0f, 0.0f);
			Vector3f eye = target - (Math::Normalize(lightDirection) * 40.0f); // Pull back 40 units
			Vector3f up = Vector3f(0.0f, 1.0f, 0.0f);
			if (std::abs(lightDirection.y) > 0.99f)
				up = Vector3f(0.0f, 0.0f, 1.0f);
			Matrix4f lightView = Math::LookAt(eye, target, up);
			m_RenderSceneState.DirectionalLightViewMatrix = lightProjection * lightView;

			// Set uniform buffer for directional light (offset 0)
			m_ShadowUniformBuffer->SetData(&m_RenderSceneState.DirectionalLightViewMatrix, sizeof(Matrix4f), 0);

			index++;
		}
		RenderGeometryForShadowMaps(registry, m_RenderSceneState.DirectionalLightViewMatrix, m_DirectionalShadowMapBuffer);
	}

	void RenderSystem::CreateSpotlightShadowMap(Registry& registry)
	{
		// Get spotlight view matrix to create shadow map
		View lightView = registry.Query<SpotLightComponent, TransformComponent>();
		unsigned int index = 0;
		for (EntityID entity : lightView)
		{
			// TODO: Will create a 4-layer texture array for spotlight shadow maps to hold multiple shadow maps in the future, 
			// but for now we will just use one shadow map and overwrite it for each spotlight. This means only one spotlight can cast shadows at a time
			if (index >= Constants::Renderer::MaxSpotLights)
				break;

			auto [light, transform] = registry.GetComponents<SpotLightComponent, TransformComponent>(entity);
			Vector3f lightDirection = transform.GetForward();

			// TODO: These props are just hard coded but will eventually move to "Dynamic Shadow Frustums" and "Cascaded Shadow Maps"
			Matrix4f lightProjection = Math::Perspective(Math::Degrees(light.OuterCutOffAngle) * 2.0f, 1.0f, 1.0f, 100.0f);
			Vector3f target = lightDirection + transform.Position;	// Look in the direction of the spotlight
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

	void RenderSystem::RenderGeometryForShadowMaps(Registry& registry, const Matrix4f& lightViewMatrix, const SharedPtr<Framebuffer>& shadowMapBuffer)
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
			auto [mesh, material, transform] = registry.GetComponents<MeshComponent, MaterialComponent, TransformComponent>(entity);
			shadowShader->SetMatrix4(Constants::Uniforms::Transform, transform.WorldTransform);
			auto meshAsset = assetManager.GetAsset<Mesh>(mesh.MeshHandle);
			Renderer3D::Submit(meshAsset->GetVertexArray());
		}

		Renderer3D::EndFrame();
	}

	void RenderSystem::RenderDeferredGeometry(Registry& registry)
	{
		m_GBuffer->Bind();
		RenderAction::SetViewport(0, 0, m_GBuffer->GetSpecification().Width, m_GBuffer->GetSpecification().Height);

		RenderAction::SetClearColor(Ember::Vector4f(0.0f, 0.0f, 0.0f, 1.0f));
		RenderAction::Clear(Ember::RendererAPI::RenderBit::Color | Ember::RendererAPI::RenderBit::Depth);
		RenderAction::UseDepthTest(true);

		// Clear EntityId attachment
		int clearValue = Constants::Entities::InvalidEntityID;
		m_GBuffer->ClearAttachment(4, clearValue);

		// Bind default white as the default texture for all units to avoid accidentally sampling from unbound texture units in the shader
		auto defaultWhite = Application::Instance().GetAssetManager().GetAsset<Texture>(Constants::Assets::DefaultWhiteTex);
		auto defaultBlack = Application::Instance().GetAssetManager().GetAsset<Texture>(Constants::Assets::DefaultBlackTex);
		auto defaultNormal = Application::Instance().GetAssetManager().GetAsset<Texture>(Constants::Assets::DefaultNormalTex);
		RenderAction::SetTextureUnit(0, defaultWhite->GetID());
		RenderAction::SetTextureUnit(1, defaultNormal->GetID());
		RenderAction::SetTextureUnit(2, defaultWhite->GetID());
		RenderAction::SetTextureUnit(3, defaultBlack->GetID());

		Renderer3D::BeginFrame();

		for (EntityID entity : m_RenderQueueBuckets.Opaque)
		{
			auto [mesh, material, transform] = registry.GetComponents<MeshComponent, MaterialComponent, TransformComponent>(entity);
			if (mesh.MeshHandle == Constants::InvalidUUID || material.MaterialHandle == Constants::InvalidUUID)
				continue;

			auto meshAsset = Application::Instance().GetAssetManager().GetAsset<Mesh>(mesh.MeshHandle);
			auto materialAsset = Application::Instance().GetAssetManager().GetAsset<MaterialBase>(material.MaterialHandle);
			materialAsset->GetShader()->Bind();
			materialAsset->GetShader()->SetInt(Constants::Uniforms::EntityID, entity);
			Renderer3D::Submit(meshAsset->GetVertexArray(), materialAsset, transform.WorldTransform);
		}

		Renderer3D::EndFrame();
	}

	void RenderSystem::RenderDeferredLighting(Registry& registry)
	{
		int dims[4] = { 0 };
		RenderAction::GetViewportDimensions(dims);
		m_RenderSceneState.ViewportDimensions = Vector4<int>(dims[0], dims[1], dims[2], dims[3]);

		RenderAction::UseDepthTest(false);
		RenderAction::UseFaceCulling(false);

		m_HdrSceneBuffer->Bind();
		RenderAction::SetViewport(m_RenderSceneState.ViewportDimensions);

		RenderAction::SetClearColor(Ember::Vector4f(0.0f, 0.0f, 0.0f, 1.0f));
		RenderAction::Clear(Ember::RendererAPI::RenderBit::Color);

		// clear entity id attachment
		int clearValue = Constants::Entities::InvalidEntityID;
		m_HdrSceneBuffer->ClearAttachment(2, clearValue);

		auto& assetManager = Application::Instance().GetAssetManager();
		auto litShader = assetManager.GetAsset<Shader>(Constants::Assets::StandardLitShad);

		litShader->Bind();
		litShader->SetFloat3(Constants::Uniforms::CameraPosition, m_RenderSceneState.CameraTransform[3]);
		litShader->SetInt(Constants::Uniforms::AlbedoRoughness, 0);
		litShader->SetInt(Constants::Uniforms::NormalMetallic, 1);
		litShader->SetInt(Constants::Uniforms::PositionAO, 2);
		litShader->SetInt(Constants::Uniforms::EmissionOut, 3);
		litShader->SetInt(Constants::Uniforms::DirectionShadowMap, 4);
		litShader->SetInt(Constants::Uniforms::SpotShadowMap, 5);

		RenderAction::SetTextureUnit(0, m_GBuffer->GetColorAttachmentID(0));
		RenderAction::SetTextureUnit(1, m_GBuffer->GetColorAttachmentID(1));
		RenderAction::SetTextureUnit(2, m_GBuffer->GetColorAttachmentID(2));
		RenderAction::SetTextureUnit(3, m_GBuffer->GetColorAttachmentID(3));
		RenderAction::SetTextureUnit(4, m_DirectionalShadowMapBuffer->GetDepthAttachmentID());
		RenderAction::SetTextureUnit(5, m_SpotShadowMapBuffer->GetDepthAttachmentID());

		LightDataBlock lightData = {};

		// Directional Lights
		{
			View view = registry.Query<DirectionalLightComponent, TransformComponent>();
			for (EntityID entity : view)
			{
				if (lightData.ActiveDirectionalLights >= Constants::Renderer::MaxDirectionalLights)
					break;

				auto [light, transform] = registry.GetComponents<DirectionalLightComponent, TransformComponent>(entity);
				int i = lightData.ActiveDirectionalLights;

				lightData.DirectionalLights[i].Direction = transform.GetForward();
				lightData.DirectionalLights[i].Color = light.Color;
				lightData.DirectionalLights[i].Intensity = light.Intensity;

				lightData.ActiveDirectionalLights++;
			}
		}

		// Spotlights
		{
			View view = registry.Query<SpotLightComponent, TransformComponent>();
			for (EntityID entity : view)
			{
				if (lightData.ActiveSpotLights >= Constants::Renderer::MaxSpotLights)
					break;

				auto [light, transform] = registry.GetComponents<SpotLightComponent, TransformComponent>(entity);
				int i = lightData.ActiveSpotLights;

				lightData.SpotLights[i].Position = transform.Position;
				lightData.SpotLights[i].Direction = transform.GetForward();
				lightData.SpotLights[i].Color = light.Color;
				lightData.SpotLights[i].Intensity = light.Intensity;
				lightData.SpotLights[i].CutOff = light.CutOff;
				lightData.SpotLights[i].OuterCutOff = light.OuterCutOff;

				lightData.ActiveSpotLights++;
			}
		}

		// Point Lights
		{
			View view = registry.Query<PointLightComponent, TransformComponent>();
			for (EntityID entity : view)
			{
				if (lightData.ActivePointLights >= Constants::Renderer::MaxPointLights)
					break;

				auto [light, transform] = registry.GetComponents<PointLightComponent, TransformComponent>(entity);
				int i = lightData.ActivePointLights;

				lightData.PointLights[i].Position = transform.Position;
				lightData.PointLights[i].Color = light.Color;
				lightData.PointLights[i].Intensity = light.Intensity;
				lightData.PointLights[i].Radius = light.Radius;

				lightData.ActivePointLights++;
			}
		}

		m_LightUniformBuffer->SetData(&lightData, sizeof(LightDataBlock), 0);
		Renderer3D::Submit(m_ScreenQuad->GetVertexArray());
	}

	void RenderSystem::RenderForwardEntities(Registry& registry)
	{
		// Copy depth buffer for forward rendering
		RenderAction::CopyDepthBuffer(m_GBuffer->GetID(), m_HdrSceneBuffer->GetID(), m_RenderSceneState.ViewportDimensions);
		m_HdrSceneBuffer->Bind();

		RenderAction::UseDepthTest(true);

		Renderer3D::BeginFrame();

		for (EntityID entity : m_RenderQueueBuckets.Forward)
		{
			auto [mesh, material, transform] = registry.GetComponents<MeshComponent, MaterialComponent, TransformComponent>(entity);
			auto meshAsset = Application::Instance().GetAssetManager().GetAsset<Mesh>(mesh.MeshHandle);
			auto materialAsset = Application::Instance().GetAssetManager().GetAsset<MaterialBase>(material.MaterialHandle);
			materialAsset->GetShader()->Bind();
			materialAsset->GetShader()->SetInt(Constants::Uniforms::EntityID, entity);
			Renderer3D::Submit(meshAsset->GetVertexArray(), materialAsset, transform.WorldTransform);
		}

		Renderer3D::EndFrame();
	}

	void RenderSystem::RenderTransparentEntities(Registry& registry)
	{

	}

	void RenderSystem::RenderInfiniteGrid()
	{
		m_HdrSceneBuffer->Bind();

		RenderAction::UseDepthTest(true);
		RenderAction::UseDepthMask(false);
		RenderAction::UseBlending(true);	// For fading away the grid lines as they get further from the camera

		auto gridShader = Application::Instance().GetAssetManager().GetAsset<Shader>(Constants::Assets::InfiniteGridShad);
		gridShader->Bind();

		// The shader needs the inverse matrices to un-project the screen pixels back into 3D world space
		Matrix4f viewProj = m_RenderSceneState.ActiveCamera.GetProjectionMatrix() * Math::Inverse(m_RenderSceneState.CameraTransform);
		gridShader->SetMatrix4(Constants::Uniforms::ViewProj, viewProj);
		gridShader->SetMatrix4(Constants::Uniforms::InverseView, m_RenderSceneState.CameraTransform); // CameraTransform already is the inverse view
		gridShader->SetMatrix4(Constants::Uniforms::InverseProjection, Math::Inverse(m_RenderSceneState.ActiveCamera.GetProjectionMatrix()));
		gridShader->SetFloat3(Constants::Uniforms::CameraPosition, m_RenderSceneState.CameraTransform[3]);

		Renderer3D::Submit(m_ScreenQuad->GetVertexArray());

		RenderAction::UseBlending(false);
		RenderAction::UseDepthMask(true);
		m_HdrSceneBuffer->Unbind();
	}

	void RenderSystem::RenderBillboards(Registry& registry)
	{
		RenderAction::UseBlending(true);

		m_HdrSceneBuffer->Bind();
		auto& assetManager = Application::Instance().GetAssetManager();
		auto billboardShader = assetManager.GetAsset<Shader>(Constants::Assets::BillboardShad);

		billboardShader->Bind();

		View view = registry.Query<BillboardComponent, TransformComponent>();
		for (EntityID entity : view)
		{
			auto [billboard, transform] = registry.GetComponents<BillboardComponent, TransformComponent>(entity);
			auto texture = assetManager.GetAsset<Texture>(billboard.TextureHandle);

			// TODO: Eventually may be able to move all this math to the GPU in the shader

			Matrix4f cameraRotation = m_RenderSceneState.CameraTransform;
			cameraRotation[3] = Vector4f(0.0f, 0.0f, 0.0f, 1.0f); // Remove translation from camera transform to only get rotation for the billboard shader
			
			Matrix4f billboardTransform;
			if (billboard.IsSpherical)
			{
				// Always faces the camera, but keeps its own position
				billboardTransform = Math::Translate(transform.Position) * cameraRotation * Math::Scale(Vector3f(0.5f));
			}
			else 
			{
				// Only want the camera's rotation on the Y axis for cylindrical billboards
				auto quat = Math::ToQuaternion(cameraRotation);
				auto euler = Math::ToEulerAngles(quat);
				float yaw = euler.y;
				billboardTransform = Math::Translate(transform.Position) * Math::Rotate(yaw, Vector3f(0.0f, 1.0f, 0.0f)) * Math::Scale(Vector3f(0.5f));
			}

			Matrix4f viewProj = m_RenderSceneState.ActiveCamera.GetProjectionMatrix() * Math::Inverse(m_RenderSceneState.CameraTransform);
			billboardShader->SetMatrix4(Constants::Uniforms::ViewProj, viewProj);
			billboardShader->SetMatrix4(Constants::Uniforms::Transform, billboardTransform);

			billboardShader->SetFloat4(Constants::Uniforms::Color, billboard.Tint);
			billboardShader->SetInt(Constants::Uniforms::EntityID, entity);

			billboardShader->SetInt(Constants::Uniforms::Image, 0);
			RenderAction::SetTextureUnit(0, texture->GetID());

			Renderer3D::Submit(PrimitiveGenerator::CreateQuad(1.0f, 1.0f)->GetVertexArray());
		}

		m_HdrSceneBuffer->Unbind();

		RenderAction::UseBlending(false);
	}

	void RenderSystem::Render2DEntities(Registry& registry)
	{
		RenderAction::UseDepthTest(false);

		Renderer2D::BeginFrame();

		View view = registry.Query<SpriteComponent, TransformComponent>();
		for (EntityID entity : view)
		{
			auto [sprite, transform] = registry.GetComponents<SpriteComponent, TransformComponent>(entity);
			if (sprite.TextureHandle == Constants::InvalidUUID)
			{
				Renderer2D::DrawQuad(Vector2f(transform.Position.x, transform.Position.y),
					Vector2f(transform.Scale.x, transform.Scale.y), sprite.Color);
			}
			else
			{
				auto textureAsset = Application::Instance().GetAssetManager().GetAsset<Texture>(sprite.TextureHandle);
				Renderer2D::DrawQuad(Vector2f(transform.Position.x, transform.Position.y),
					Vector2f(transform.Scale.x, transform.Scale.y), sprite.Color, textureAsset);
			}
		}

		Renderer2D::EndFrame();
	}

	void RenderSystem::HandlePostProcessing(Registry& registry)
	{
		RenderAction::UseDepthTest(false);

		SharedPtr<Framebuffer> currentInput = m_HdrSceneBuffer;
		SharedPtr<Framebuffer> currentOutput = m_PostProcessBufferA;

		// TODO: Need a more elegant way to handle these special VFX cases
		// Grab outline components for selected entities
		std::unordered_map<EntityID, OutlineComponent> outlinedEntityMap;
		View view = registry.Query<OutlineComponent>();
		for (EntityID entity : view)
		{
			auto [outline] = registry.GetComponents<OutlineComponent>(entity);
			outlinedEntityMap[entity] = outline;
		}


		// Pass over all post processing items
		for (auto& pass : m_PostProcessStack)
		{
			// TODO: Come up with solution for handling special cases
			if (auto outlinePass = DynamicPointerCast<OutlinePass>(pass))
			{
				if (pass->Enabled)
				{
					// Special case for outline pass since it needs the G-Buffer as well as the scene buffer
					for (const auto& [entityID, outline] : outlinedEntityMap)
					{
						outlinePass->SetGBuffer(m_GBuffer);
						outlinePass->SetHdrBuffer(m_HdrSceneBuffer);
						outlinePass->SetSelectedEntityID(entityID);
						outlinePass->SetOutlineColor(outline.Color);
						outlinePass->SetOutlineThickness(outline.Thickness);

						pass->Render(currentInput, currentOutput);
						currentInput = currentOutput;
						currentOutput = (currentOutput == m_PostProcessBufferA) ? m_PostProcessBufferB : m_PostProcessBufferA;
					}
				}
				
				continue;
			}

			if (pass->Enabled)
			{
				pass->Render(currentInput, currentOutput);
				currentInput = currentOutput;
				currentOutput = (currentOutput == m_PostProcessBufferA) ? m_PostProcessBufferB : m_PostProcessBufferA;
			}
		}

		RenderFinalComposite(currentInput);
	}

	void RenderSystem::RenderFinalComposite(const SharedPtr<Framebuffer>& outputBuffer)
	{
		// Final blit targeting the output buffer
		RenderAction::SetFramebuffer(m_RenderSceneState.OutputFramebufferId);
		RenderAction::SetViewport(m_RenderSceneState.ViewportDimensions);
		RenderAction::Clear(Ember::RendererAPI::RenderBit::Color | Ember::RendererAPI::RenderBit::Depth);
		RenderAction::UseDepthTest(false);

		// Bind the Final Composite Shader
		auto finalShader = Application::Instance().GetAssetManager().GetAsset<Shader>(Constants::Assets::FinalCompositeShad);
		finalShader->Bind();

		// Set the exposure (hook this up to an ImGui slider later)
		finalShader->SetFloat(Constants::Uniforms::Exposure, 1.0f);
		finalShader->SetInt(Constants::Uniforms::Scene, 0);

		RenderAction::SetTextureUnit(0, outputBuffer->GetColorAttachmentID(0));

		Renderer3D::Submit(m_ScreenQuad->GetVertexArray());
	}

	void RenderSystem::ResetRenderState()
	{
		RenderAction::UseDepthTest(true);
	}

	void RenderSystem::SortEntitiesByRenderQueue(Registry& registry)
	{
		View view = registry.Query<MeshComponent, MaterialComponent, TransformComponent>();
		for (EntityID entity : view)
		{
			auto [mesh, material, transform] = registry.GetComponents<MeshComponent, MaterialComponent, TransformComponent>(entity);
			if (mesh.MeshHandle == Constants::InvalidUUID || material.MaterialHandle == Constants::InvalidUUID)
				continue;

			auto materialAsset = Application::Instance().GetAssetManager().GetAsset<MaterialBase>(material.MaterialHandle);
			switch (materialAsset->GetRenderQueue())
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