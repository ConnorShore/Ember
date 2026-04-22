#include "ebpch.h"
#include "RenderSystem.h"
#include "PhysicsSystem.h"
#include "ParticleSystem.h"

#include "Ember/Core/Application.h"
#include "Ember/ECS/Component/Components.h"
#include "Ember/Render/RenderAction.h"
#include "Ember/Render/Renderer2D.h"
#include "Ember/Render/Renderer3D.h"
#include "Ember/Render/PrimitiveGenerator.h"
#include "Ember/Render/VFX/BloomPass.h"
#include "Ember/Render/VFX/OutlinePass.h"
#include "Ember/Render/VFX/FXAAPass.h"
#include "Ember/Render/DebugRenderer.h"
#include "Ember/Scene/Scene.h"

namespace Ember {

	// UBO light data structs - layout matches std140 in the lighting shader
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

		// GBuffer: packed deferred rendering targets
		// [0] AlbedoRoughness  [1] NormalMetallic  [2] PositionAO
		// [3] Emission  [4] EntityID (integer for picking)  [5] Depth
		{
			Ember::FramebufferSpecification specs;
			specs.Width = 1;
			specs.Height = 1;
			specs.AttachmentSpecs = {
				Ember::FramebufferTextureFormat::RGBA8,				// AlbedoRoughness
				Ember::FramebufferTextureFormat::RGBA16F,			// NormalMetallic
				Ember::FramebufferTextureFormat::RGBA16F,			// PositionAO
				Ember::FramebufferTextureFormat::RGBA16F,			// Emission
				Ember::FramebufferTextureFormat::RedInteger,		// EntityID
				Ember::FramebufferTextureFormat::Depth24Stencil8	// Depth
			};
			m_GBuffer = Framebuffer::Create(specs);
		}

		// Direction ShadowMap Buffer
		{
			Ember::FramebufferSpecification specs;
			specs.Width = 2048;
			specs.Height = 2048;
			specs.AttachmentSpecs = {
				Ember::FramebufferTextureFormat::Depth24Stencil8
			};
			m_DirectionalShadowMapBuffer = Framebuffer::Create(specs);
		}
		// Spot ShadowMap Buffer
		{
			Ember::FramebufferSpecification specs;
			specs.Width = 2048;
			specs.Height = 2048;
			specs.AttachmentSpecs = {
				Ember::FramebufferTextureFormat::Depth24Stencil8
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
				Ember::FramebufferTextureFormat::RedInteger,
				Ember::FramebufferTextureFormat::Depth24Stencil8
			};
			m_HdrSceneBuffer = Framebuffer::Create(specs);
			m_PostProcessBufferA = Framebuffer::Create(specs);
			m_PostProcessBufferB = Framebuffer::Create(specs);
		}
		// LDR Post Process Buffers (For FXAA, Vignette, etc.)
		{
			Ember::FramebufferSpecification specs;
			specs.Width = 1;
			specs.Height = 1;
			specs.AttachmentSpecs = {
				Ember::FramebufferTextureFormat::RGBA8, // Standard 0-1 colors!
				Ember::FramebufferTextureFormat::Depth24Stencil8
			};
			m_LdrBufferA = Framebuffer::Create(specs);
			m_LdrBufferB = Framebuffer::Create(specs);
		}

		// Uniform Buffer Objects at fixed binding points shared across all shaders
		m_CameraUniformBuffer = UniformBuffer::Create(sizeof(Matrix4f), 0);          // binding 0: ViewProjection
		m_ShadowUniformBuffer = UniformBuffer::Create(sizeof(Matrix4f) * 2, 1);      // binding 1: DirLight + SpotLight VP
		m_LightUniformBuffer = UniformBuffer::Create(sizeof(LightDataBlock), 2);     // binding 2: All light data

		m_ScreenQuad = PrimitiveGenerator::CreateQuad(2.0f, 2.0f);

		//////////////// Init post processing stack ////////////////////////
		m_PostProcessStack.emplace_back(SharedPtr<BloomPass>::Create());

		auto outlinePass = SharedPtr<OutlinePass>::Create();
		outlinePass->SetGBuffer(m_GBuffer);
		m_PostProcessStack.emplace_back(outlinePass);

		m_PostProcessStack.emplace_back(SharedPtr<FXAAPass>::Create());
		////////////////////////////////////////////////////////////////////

		m_Skybox = SharedPtr<Skybox>::Create(Constants::Assets::DefaultSkyboxUUID);
		RenderAction::UseCubeMapSeamless(true);

		for (auto& pass : m_PostProcessStack)
			pass->Init();

		// Debug Drawing
		uint32_t maxDebugVertices = 20000;
		m_PhysicsDebugLineVBO = VertexBuffer::Create(maxDebugVertices * sizeof(DebugVertex));

		m_PhysicsDebugLineVBO->SetLayout({
			{ ShaderDataType::Float3, "v_Position" },
			{ ShaderDataType::Float4, "v_Color" }
			});

		m_PhysicsDebugLineVAO = VertexArray::Create();

		// Notice we dropped the '0' here, using our fixed method!
		m_PhysicsDebugLineVAO->AddVertexBuffer(m_PhysicsDebugLineVBO);

		// Particle setup
		auto quadVAO = PrimitiveGenerator::CreateQuad(1.0f, 1.0f)->GetVertexArray();
		auto quadVBO = quadVAO->GetVertexBuffer();
		auto quadIBO = quadVAO->GetIndexBuffer();
		m_ParticleVBO = VertexBuffer::Create(Constants::Renderer::MaxParticles * sizeof(ParticleVertex));
		m_ParticleVBO->SetLayout({
			{ ShaderDataType::Float3, "i_Position", true /* Instanced */ },
			{ ShaderDataType::Float, "i_Scale", true  /* Instanced */ },
			{ ShaderDataType::Float4, "i_Color", true  /* Instanced */ }
		});

		m_ParticleVAO = VertexArray::Create();
		m_ParticleVAO->AddVertexBuffer(quadVBO);
		m_ParticleVAO->AddVertexBuffer(m_ParticleVBO);
		m_ParticleVAO->SetIndexBuffer(quadIBO);

		m_RenderSceneState.Reset();
		EB_CORE_INFO("RenderSystem is attached!");
	}

	void RenderSystem::OnDetach()
	{
		Renderer2D::Shutdown();
		Renderer3D::Shutdown();
		EB_CORE_INFO("RenderSystem is detached!");
	}

	void RenderSystem::ExecuteRenderPipeline(Scene* scene, bool isRuntime)
	{
		RenderAction::GetPreviousFramebuffer(&m_RenderSceneState.OutputFramebufferId);

		if (!m_RenderSceneState.IsCameraFound)
			return;

		SortEntitiesByRenderQueue(scene);

		// --- Shadow pass ---
		CreateDirectionalShadowMap(scene);
		CreateSpotlightShadowMap(scene);

		// --- Deferred pipeline: geometry into GBuffer, then full-screen lighting resolve ---
		RenderDeferredGeometry(scene);
		RenderDeferredLighting(scene);

		// Blit GBuffer depth into HDR buffer so forward objects are properly depth-tested
		RenderAction::CopyDepthBuffer(m_GBuffer->GetID(), m_HdrSceneBuffer->GetID(), m_RenderSceneState.ViewportDimensions);

		// --- Skybox ---
		if (m_Skybox->Enabled())
			RenderSkybox(scene);

		// --- Forward pipeline: depth-tested draws on top of the deferred result ---
		RenderForwardEntities(scene);
		RenderTransparentEntities(scene);

		if (!isRuntime)
			RenderInfiniteGrid();

		RenderParticles(scene);
		RenderBillboards(scene, isRuntime);

		// Draw World-Space 2D BEFORE Post-Processing
		RenderWorldSpace2D(scene);

		// Post Processing & Tone Mapping
		HandlePostProcessing(scene);

		// Debug lines
		RenderDebug(scene);

		// Draw Screen-Space UI AFTER Final Composite
		RenderScreenSpaceUI(scene);

		// Reset any modified render state so other systems aren't affected (like the Editor's Gizmo system)
		ResetRenderState();
	}

	void RenderSystem::OnUpdate(TimeStep delta, Scene* scene)
	{
		m_CurrentScene = scene;
		InitializeRenderState();
		SetSceneCamera(scene);

		if (m_RenderSceneState.IsCameraFound)
			ExecuteRenderPipeline(scene, true);

		m_CurrentScene = nullptr;
	}

	void RenderSystem::OnUpdate(TimeStep delta, Scene* scene, const Camera& camera, const Matrix4f& cameraTransform)
	{
		m_CurrentScene = scene;
		InitializeRenderState();

		// Set render scene state for camera info
		m_RenderSceneState.ActiveCamera = camera;
		m_RenderSceneState.CameraTransform = cameraTransform;
		m_RenderSceneState.IsCameraFound = true;

		Matrix4f viewProjectionMat = camera.GetProjectionMatrix() * Math::Inverse(cameraTransform);
		m_CameraUniformBuffer->SetData(&viewProjectionMat, sizeof(Matrix4f));

		// Update the system
		ExecuteRenderPipeline(scene, false);

		m_CurrentScene = nullptr;
	}

	void RenderSystem::OnViewportResize(uint32_t width, uint32_t height)
	{
		m_GBuffer->ViewportResize(width, height);
		m_HdrSceneBuffer->ViewportResize(width, height);
		m_PostProcessBufferA->ViewportResize(width, height);
		m_PostProcessBufferB->ViewportResize(width, height);

		m_LdrBufferA->ViewportResize(width, height);
		m_LdrBufferB->ViewportResize(width, height);

		for (auto& pass : m_PostProcessStack)
			pass->OnViewportResize(width, height);
	}

	// Reads the entity ID from the framebuffer at a pixel. Checks the forward buffer
	// first (drawn on top), falling back to the GBuffer's entity ID attachment.
	EntityID RenderSystem::GetEntityIDAtPixel(uint32_t x, uint32_t y)
	{
		// Check the Forward buffer first (since it is drawn on top of the world)
		m_HdrSceneBuffer->Bind();
		int forwardPixelData = m_HdrSceneBuffer->ReadPixel(2, x, y);
		m_HdrSceneBuffer->Unbind();

		if (forwardPixelData != Constants::Entities::InvalidEntityID)
			return (EntityID)forwardPixelData;

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

	void RenderSystem::SetSceneCamera(Scene* scene)
	{
		auto& registry = scene->GetRegistry();
		View cameraView = registry.ActiveQuery<CameraComponent, TransformComponent>();
		for (EntityID cameraEntity : cameraView)
		{
			auto [camera, transform] = registry.GetComponents<CameraComponent, TransformComponent>(cameraEntity);
			if (camera.IsActive)
			{
				m_RenderSceneState.ActiveCamera = camera.Camera;
				m_RenderSceneState.CameraTransform = transform.WorldTransform;// Math::Translate(transform.Position)* Math::GetRotationMatrix(transform.Rotation);
				m_RenderSceneState.IsCameraFound = true;

				// set uniform buffer
				Matrix4f viewProjectionMat = camera.Camera.GetProjectionMatrix() * Math::Inverse(m_RenderSceneState.CameraTransform);
				m_CameraUniformBuffer->SetData(&viewProjectionMat, sizeof(Matrix4f));

				break;
			}
		}
	}

	void RenderSystem::CreateShadowMaps(Scene* scene)
	{
		CreateDirectionalShadowMap(scene);
		CreateSpotlightShadowMap(scene);
	}

	void RenderSystem::CreateDirectionalShadowMap(Scene* scene)
	{
		auto& registry = scene->GetRegistry();

		// Get directional light view matrix to create shadow map
		View lightView = registry.ActiveQuery<DirectionalLightComponent, TransformComponent>();
		uint32_t index = 0;
		for (EntityID entity : lightView)
		{
			if (index >= Constants::Renderer::MaxDirectionalLights)
				break;

			auto [light, transform] = registry.GetComponents<DirectionalLightComponent, TransformComponent>(entity);
			Vector3f lightDirection = transform.GetForward();

			// TODO: These props are just hard coded but will eventually move to "Dynamic Shadow Frustums" and "Cascaded Shadow Maps"
			//Matrix4f lightProjection = Math::Orthographic(-35.0f, 35.0f, -35.0f, 35.0f, 1.0f,500.0f);
			Matrix4f lightProjection = Math::Orthographic(-25.0f, 25.0f, -25.0f, 25.0f, -20.0f, 200.0f);

			Vector3f target = Vector3f(0.0f, 0.0f, 0.0f);
			Vector3f eye = target - (Math::Normalize(lightDirection) * 40.0f); // Pull back 40 units
			Vector3f up = Vector3f(0.0f, 1.0f, 0.0f);

			// Avoid degenerate LookAt when light points straight up/down
			if (std::abs(lightDirection.y) > 0.99f)
				up = Vector3f(0.0f, 0.0f, 1.0f);

			Matrix4f lightView = Math::LookAt(eye, target, up);
			m_RenderSceneState.DirectionalLightViewMatrix = lightProjection * lightView;

			// Set uniform buffer for directional light (offset 0)
			m_ShadowUniformBuffer->SetData(&m_RenderSceneState.DirectionalLightViewMatrix, sizeof(Matrix4f), 0);

			index++;
		}
		RenderGeometryForShadowMaps(scene, m_RenderSceneState.DirectionalLightViewMatrix, m_DirectionalShadowMapBuffer);
	}

	void RenderSystem::CreateSpotlightShadowMap(Scene* scene)
	{
		auto& registry = scene->GetRegistry();

		// Get spotlight view matrix to create shadow map
		View lightView = registry.ActiveQuery<SpotLightComponent, TransformComponent>();
		uint32_t index = 0;
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
			Vector3f worldPos = Vector3f(transform.WorldTransform[3]);
			Vector3f target = lightDirection + worldPos;	// Look in the direction of the spotlight
			Vector3f eye = worldPos;
			Vector3f up = Vector3f(0.0f, 1.0f, 0.0f);
			Matrix4f lightView = Math::LookAt(eye, target, up);
			m_RenderSceneState.SpotLightViewMatrix = lightProjection * lightView;

			// Set uniform buffer for spotlight (offset -> 1 mat4)
			m_ShadowUniformBuffer->SetData(&m_RenderSceneState.SpotLightViewMatrix, sizeof(Matrix4f), sizeof(Matrix4f));

			index++;
		}

		RenderGeometryForShadowMaps(scene, m_RenderSceneState.SpotLightViewMatrix, m_SpotShadowMapBuffer);
	}

	void RenderSystem::RenderGeometryForShadowMaps(Scene* scene, const Matrix4f& lightViewMatrix, const SharedPtr<Framebuffer>& shadowMapBuffer)
	{
		auto& registry = scene->GetRegistry();
		auto& assetManager = Application::Instance().GetAssetManager();

		shadowMapBuffer->Bind();

		RenderAction::SetViewport(0, 0, shadowMapBuffer->GetSpecification().Width, shadowMapBuffer->GetSpecification().Height);
		RenderAction::Clear(Ember::RendererAPI::RenderBit::Depth);
		RenderAction::UseDepthTest(true);

		// Split entities  so can bind each shader 1 time
		int size = (int)m_RenderQueueBuckets.Opaque.size();
		std::vector<EntityID> splitEntities(size);

		int staticCount = 0;
		int skinnedCount = 0;
		for (int i = 0; i < size; i++)
		{
			EntityID entity = m_RenderQueueBuckets.Opaque[i];
			if (registry.ContainsComponent<SkinnedMeshComponent>(entity))
				splitEntities[size - 1 - skinnedCount++] = entity; // Add to end of list
			else if (registry.ContainsComponent<StaticMeshComponent>(entity))
				splitEntities[staticCount++] = entity; // Keep at beginning of list
		}


		Renderer3D::BeginFrame();

		// Render static meshes with the static shadow shader
		auto shadowShader = assetManager.GetAsset<Shader>(Constants::Assets::StandardShadowShad);
		shadowShader->Bind();
		shadowShader->SetMatrix4(Constants::Uniforms::LightViewMatrix, lightViewMatrix);

		for (int i = 0; i < staticCount; i++)
		{
			EntityID entity = splitEntities[i];
			auto [transform] = registry.GetComponents<TransformComponent>(entity);
			shadowShader->SetMatrix4(Constants::Uniforms::Transform, transform.WorldTransform);
			if (registry.ContainsComponent<StaticMeshComponent>(entity))
			{
				auto& mesh = registry.GetComponent<StaticMeshComponent>(entity);
				auto meshAsset = assetManager.GetAsset<Mesh>(mesh.MeshHandle);
				Renderer3D::Submit(meshAsset->GetVertexArray());
			}
		}

		// Render skinned meshes with the skinned shadow shader
		auto shadowSkinnedShader = assetManager.GetAsset<Shader>(Constants::Assets::StandardSkinnedShadowShad);
		shadowSkinnedShader->Bind();
		shadowSkinnedShader->SetMatrix4(Constants::Uniforms::LightViewMatrix, lightViewMatrix);

		for (int i = size - skinnedCount; i < size; i++)
		{
			EntityID entity = splitEntities[i];
			auto [transform] = registry.GetComponents<TransformComponent>(entity);
			shadowSkinnedShader->SetMatrix4(Constants::Uniforms::Transform, transform.WorldTransform);
			if (registry.ContainsComponent<SkinnedMeshComponent>(entity))
			{
				auto& mesh = registry.GetComponent<SkinnedMeshComponent>(entity);
				auto meshAsset = assetManager.GetAsset<Mesh>(mesh.MeshHandle);
				if (mesh.AnimatorEntityHandle != Constants::InvalidUUID && m_CurrentScene)
				{
					Entity animatorEntity = m_CurrentScene->GetEntity(mesh.AnimatorEntityHandle);
					if (animatorEntity.GetEntityHandle() != Constants::Entities::InvalidEntityID)
					{
						auto& animator = registry.GetComponent<AnimatorComponent>(animatorEntity.GetEntityHandle());
						shadowSkinnedShader->SetMatrix4Array(Constants::Uniforms::BoneMatrices, animator.BoneMatrices.data(), static_cast<uint32_t>(animator.BoneMatrices.size()));
					}
				}
				Renderer3D::Submit(meshAsset->GetVertexArray());
			}
		}

		Renderer3D::EndFrame();
	}

	void RenderSystem::RenderDeferredGeometry(Scene* scene)
	{
		auto& registry = scene->GetRegistry();

		m_GBuffer->Bind();
		RenderAction::SetViewport(0, 0, m_GBuffer->GetSpecification().Width, m_GBuffer->GetSpecification().Height);

		RenderAction::SetClearColor(Ember::Vector4f(0.0f, 0.0f, 0.0f, 1.0f));
		RenderAction::Clear(Ember::RendererAPI::RenderBit::Color | Ember::RendererAPI::RenderBit::Depth);
		RenderAction::UseDepthTest(true);

		// Clear EntityId attachment
		int clearValue = Constants::Entities::InvalidEntityID;
		m_GBuffer->ClearAttachment(4, clearValue);

		// Bind default white as the default texture for all units to avoid accidentally sampling from unbound texture units in the shader
		auto defaultWhite = Application::Instance().GetAssetManager().GetAsset<Texture2D>(Constants::Assets::DefaultWhiteTex);
		auto defaultBlack = Application::Instance().GetAssetManager().GetAsset<Texture2D>(Constants::Assets::DefaultBlackTex);
		auto defaultNormal = Application::Instance().GetAssetManager().GetAsset<Texture2D>(Constants::Assets::DefaultNormalTex);
		RenderAction::SetTextureUnit(0, defaultWhite->GetID());
		RenderAction::SetTextureUnit(1, defaultNormal->GetID());
		RenderAction::SetTextureUnit(2, defaultWhite->GetID());
		RenderAction::SetTextureUnit(3, defaultBlack->GetID());

		Renderer3D::BeginFrame();

		for (EntityID entity : m_RenderQueueBuckets.Opaque)
		{
			auto [material, transform] = registry.GetComponents<MaterialComponent, TransformComponent>(entity);
			if (material.MaterialHandle == Constants::InvalidUUID)
				continue;

			auto materialAsset = Application::Instance().GetAssetManager().GetAsset<MaterialBase>(material.MaterialHandle);
			materialAsset->GetShader()->Bind();
			materialAsset->GetShader()->SetInt(Constants::Uniforms::EntityID, entity);

			if (registry.ContainsComponent<StaticMeshComponent>(entity))
			{
				auto& mesh = registry.GetComponent<StaticMeshComponent>(entity);
				if (mesh.MeshHandle == Constants::InvalidUUID)
					continue;

				auto meshAsset = Application::Instance().GetAssetManager().GetAsset<Mesh>(mesh.MeshHandle);
				Renderer3D::Submit(meshAsset->GetVertexArray(), materialAsset, transform.WorldTransform);
			}
			else if (registry.ContainsComponent<SkinnedMeshComponent>(entity))
			{
				auto& mesh = registry.GetComponent<SkinnedMeshComponent>(entity);
				if (mesh.MeshHandle == Constants::InvalidUUID)
					continue;

				auto meshAsset = Application::Instance().GetAssetManager().GetAsset<Mesh>(mesh.MeshHandle);

				// Cache runtime animator Id -> do expensive lookup only once
				if (mesh.RuntimeAnimatorID == Constants::Entities::InvalidEntityID && mesh.AnimatorEntityHandle != Constants::InvalidUUID)
				{
					Entity animatorEnt = scene->GetEntity(mesh.AnimatorEntityHandle);
					if (animatorEnt.GetEntityHandle() != Constants::Entities::InvalidEntityID)
					{
						mesh.RuntimeAnimatorID = animatorEnt.GetEntityHandle();
					}
				}

				// Use cached runtime animator id
				if (mesh.RuntimeAnimatorID != Constants::Entities::InvalidEntityID)
				{
					EB_CORE_ASSERT(registry.ContainsComponent<AnimatorComponent>(mesh.RuntimeAnimatorID), "Animator component should be present");

 					auto& animator = registry.GetComponent<AnimatorComponent>(mesh.RuntimeAnimatorID);
					materialAsset->GetShader()->SetMatrix4Array(Constants::Uniforms::BoneMatrices, animator.BoneMatrices.data(), static_cast<uint32_t>(animator.BoneMatrices.size()));
				}

				Renderer3D::Submit(meshAsset->GetVertexArray(), materialAsset, transform.WorldTransform);
			}
		}

		Renderer3D::EndFrame();
	}

	void RenderSystem::RenderDeferredLighting(Scene* scene)
	{
		auto& registry = scene->GetRegistry();

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

		if (m_Skybox->Enabled())
			litShader->SetFloat(Constants::Uniforms::EnvironmentIntensity, m_Skybox->GetIntensity());
		else
			litShader->SetFloat(Constants::Uniforms::EnvironmentIntensity, 0.0f);

		litShader->SetInt(Constants::Uniforms::AlbedoRoughness, 0);
		litShader->SetInt(Constants::Uniforms::NormalMetallic, 1);
		litShader->SetInt(Constants::Uniforms::PositionAO, 2);
		litShader->SetInt(Constants::Uniforms::EmissionOut, 3);
		litShader->SetInt(Constants::Uniforms::DirectionShadowMap, 4);
		litShader->SetInt(Constants::Uniforms::SpotShadowMap, 5);
		litShader->SetInt(Constants::Uniforms::IrradianceMap, 6);
		litShader->SetInt(Constants::Uniforms::PrefilterMap, 7);
		litShader->SetInt(Constants::Uniforms::BRDFLUT, 8);

		RenderAction::SetTextureUnit(0, m_GBuffer->GetColorAttachmentID(0));
		RenderAction::SetTextureUnit(1, m_GBuffer->GetColorAttachmentID(1));
		RenderAction::SetTextureUnit(2, m_GBuffer->GetColorAttachmentID(2));
		RenderAction::SetTextureUnit(3, m_GBuffer->GetColorAttachmentID(3));
		RenderAction::SetTextureUnit(4, m_DirectionalShadowMapBuffer->GetDepthAttachmentID());
		RenderAction::SetTextureUnit(5, m_SpotShadowMapBuffer->GetDepthAttachmentID());
		RenderAction::SetTextureUnit(6, m_Skybox->GetIrradianceMapID());
		RenderAction::SetTextureUnit(7, m_Skybox->GetPrefilteredMapID());
		RenderAction::SetTextureUnit(8, m_Skybox->GetBRDFLUTID());

		LightDataBlock lightData = {};

		// Directional Lights
		{
			View view = registry.ActiveQuery<DirectionalLightComponent, TransformComponent>();
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
			View view = registry.ActiveQuery<SpotLightComponent, TransformComponent>();
			for (EntityID entity : view)
			{
				if (lightData.ActiveSpotLights >= Constants::Renderer::MaxSpotLights)
					break;

				auto [light, transform] = registry.GetComponents<SpotLightComponent, TransformComponent>(entity);
				int i = lightData.ActiveSpotLights;

				lightData.SpotLights[i].Position = Vector3f(transform.WorldTransform[3]);
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
			View view = registry.ActiveQuery<PointLightComponent, TransformComponent>();
			for (EntityID entity : view)
			{
				if (lightData.ActivePointLights >= Constants::Renderer::MaxPointLights)
					break;

				auto [light, transform] = registry.GetComponents<PointLightComponent, TransformComponent>(entity);
				int i = lightData.ActivePointLights;

				lightData.PointLights[i].Position = Vector3f(transform.WorldTransform[3]);
				lightData.PointLights[i].Color = light.Color;
				lightData.PointLights[i].Intensity = light.Intensity;
				lightData.PointLights[i].Radius = light.Radius;

				lightData.ActivePointLights++;
			}
		}

		m_LightUniformBuffer->SetData(&lightData, sizeof(LightDataBlock), 0);
		Renderer3D::Submit(m_ScreenQuad->GetVertexArray());
	}

	void RenderSystem::RenderSkybox(Scene* scene)
	{
		m_HdrSceneBuffer->Bind();

		auto& assetManager = Application::Instance().GetAssetManager();
		auto cubeVao = assetManager.GetAsset<Mesh>(Constants::Assets::CubeMeshUUID)->GetVertexArray();

		// Change depth function so the skybox (which will have a depth of 1.0) passes the depth test
		RenderAction::UseDepthFunction(Ember::RendererAPI::DepthFunction::LessEqual);
		RenderAction::UseDepthTest(true);
		RenderAction::UseFaceCulling(false); // Make sure we can see the inside of the cube

		auto skyboxShader = Application::Instance().GetAssetManager().GetAsset<Shader>(Constants::Assets::SkyboxShad);
		skyboxShader->Bind();

		skyboxShader->SetMatrix4(Constants::Uniforms::Projection, m_RenderSceneState.ActiveCamera.GetProjectionMatrix());
		skyboxShader->SetMatrix4(Constants::Uniforms::View, Math::Inverse(m_RenderSceneState.CameraTransform));

		// Bind the cubemap
		RenderAction::SetTextureUnit(0, m_Skybox->GetEnvironmentCubeMapID());
		skyboxShader->SetInt(Constants::Uniforms::EnvironmentMap, 0);

		skyboxShader->SetInt(Constants::Uniforms::EntityID, Constants::Entities::InvalidEntityID);

		Renderer3D::Submit(cubeVao);

		// Reset depth function to default
		RenderAction::UseDepthFunction(Ember::RendererAPI::DepthFunction::Less);
		m_HdrSceneBuffer->Unbind();
	}

	void RenderSystem::RenderForwardEntities(Scene* scene)
	{
		auto& registry = scene->GetRegistry();

		m_HdrSceneBuffer->Bind();

		RenderAction::UseDepthTest(true);

		Renderer3D::BeginFrame();

		for (EntityID entity : m_RenderQueueBuckets.Forward)
		{
			auto [material, transform] = registry.GetComponents<MaterialComponent, TransformComponent>(entity);
			if (material.MaterialHandle == Constants::InvalidUUID)
				continue;

			auto materialAsset = Application::Instance().GetAssetManager().GetAsset<MaterialBase>(material.MaterialHandle);
			materialAsset->GetShader()->Bind();
			materialAsset->GetShader()->SetInt(Constants::Uniforms::EntityID, entity);

			if (registry.ContainsComponent<StaticMeshComponent>(entity))
			{
				auto& mesh = registry.GetComponent<StaticMeshComponent>(entity);
				auto meshAsset = Application::Instance().GetAssetManager().GetAsset<Mesh>(mesh.MeshHandle);
				Renderer3D::Submit(meshAsset->GetVertexArray(), materialAsset, transform.WorldTransform);
			}
			else if (registry.ContainsComponent<SkinnedMeshComponent>(entity))
			{
				auto& mesh = registry.GetComponent<SkinnedMeshComponent>(entity);
				auto meshAsset = Application::Instance().GetAssetManager().GetAsset<Mesh>(mesh.MeshHandle);

				if (mesh.AnimatorEntityHandle != Constants::InvalidUUID && m_CurrentScene)
				{
                  Entity animatorEntity = m_CurrentScene->GetEntity(mesh.AnimatorEntityHandle);
					if (animatorEntity.GetEntityHandle() != Constants::Entities::InvalidEntityID && registry.ContainsComponent<AnimatorComponent>(animatorEntity.GetEntityHandle()))
					{
                        auto& animator = registry.GetComponent<AnimatorComponent>(animatorEntity.GetEntityHandle());
						materialAsset->GetShader()->SetMatrix4Array(Constants::Uniforms::BoneMatrices, animator.BoneMatrices.data(), static_cast<uint32_t>(animator.BoneMatrices.size()));
					}
				}
				Renderer3D::Submit(meshAsset->GetVertexArray(), materialAsset, transform.WorldTransform);
			}
		}

		Renderer3D::EndFrame();
	}

	void RenderSystem::RenderTransparentEntities(Scene* scene)
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

	void RenderSystem::RenderParticles(Scene* scene)
	{
		auto& particleManager = Application::Instance().GetSystem<ParticleSystem>()->GetParticleManager();
		const auto& pool = particleManager.GetParticles();

		// 1. Pack the active data
		std::vector<ParticleVertex> instanceData;
		instanceData.reserve(pool.size());

		for (const auto& particle : pool)
		{
			if (!particle.Active) continue;

			ParticleVertex data;
			data.Position = particle.Position;
			data.Scale = particle.CurrentScale; // Assuming uniform scaling for now
			data.Color = particle.CurrentColor;

			instanceData.push_back(data);
		}

		uint32_t dataSize = (uint32_t)(instanceData.size() * sizeof(ParticleVertex));

		if (dataSize == 0) return; // Nothing to draw!

		// 2. Upload to the GPU (Requires adding a SetData method to your VertexBuffer API)
		m_ParticleVBO->SetData(instanceData.data(), dataSize);

		// 3. Render state
		RenderAction::UseBlending(true);
		RenderAction::UseDepthMask(false); // Read depth, but don't write depth!

		m_HdrSceneBuffer->Bind();

		auto particleShad = Application::Instance().GetAssetManager().GetAsset<Shader>(Constants::Assets::ParticleShad);
		particleShad->Bind();

		// The shader needs the camera up/right vectors to build the billboard matrix!
		Vector3f camRight = Vector3f(m_RenderSceneState.CameraTransform[0]);
		Vector3f camUp = Vector3f(m_RenderSceneState.CameraTransform[1]);
		particleShad->SetFloat3("u_CameraRight", camRight);
		particleShad->SetFloat3("u_CameraUp", camUp);

		// 4. THE SINGLE DRAW CALL
		uint32_t indexCount = m_ParticleVAO->GetIndexBuffer()->GetCount();
		uint32_t instanceCount = (uint32_t)instanceData.size();

		RenderAction::DrawIndexedInstanced(m_ParticleVAO, indexCount, instanceCount);

		m_HdrSceneBuffer->Unbind();

		RenderAction::UseDepthMask(true);
		RenderAction::UseBlending(false);
	}

	void RenderSystem::RenderBillboards(Scene* scene, bool isRuntime)
	{
		auto& registry = scene->GetRegistry();

		RenderAction::UseBlending(true);
		RenderAction::UseDepthTest(false);

		m_HdrSceneBuffer->Bind();
		auto& assetManager = Application::Instance().GetAssetManager();
		auto billboardShader = assetManager.GetAsset<Shader>(Constants::Assets::BillboardShad);

		billboardShader->Bind();

		View view = registry.ActiveQuery<BillboardComponent, TransformComponent>();
		for (EntityID entity : view)
		{
			auto [billboard, transform] = registry.GetComponents<BillboardComponent, TransformComponent>(entity);
			if (isRuntime && !billboard.RenderRuntime)
				continue;

			auto texture = assetManager.GetAsset<Texture2D>(billboard.TextureHandle);

			Vector3f worldPos, worldRot, worldScale;
			Math::DecomposeTransform(transform.WorldTransform, worldPos, worldRot, worldScale);

			// Find the billboards transform //
			Matrix4f cameraRotation = m_RenderSceneState.CameraTransform;
			cameraRotation[3] = Vector4f(0.0f, 0.0f, 0.0f, 1.0f); // Remove translation from camera transform

			// Scale billboard depending on if its static or not
			float distanceScale = billboard.Size;
			if (billboard.StaticSize)
			{
				float distance = Math::Length(worldPos - Vector3f(m_RenderSceneState.CameraTransform[3]));
				distanceScale = distance / 10.0f;
			}

			Vector3f finalScale = worldScale * distanceScale;

			Matrix4f billboardTransform;
			if (billboard.Spherical)
			{
				// Always faces the camera, but keeps its own position (using worldPos!)
				billboardTransform = Math::Translate(worldPos) * cameraRotation * Math::Scale(finalScale);
			}
			else
			{
				// Only want the camera's rotation on the Y axis for cylindrical billboards
				Vector3f cameraPos = Vector3f(m_RenderSceneState.CameraTransform[3]);
				Vector3f dirToCamera = cameraPos - worldPos;

				// Use atan2 to get the exact angle on the XZ plane
				float yaw = std::atan2(dirToCamera.x, dirToCamera.z);

				billboardTransform = Math::Translate(worldPos) * Math::Rotate(yaw, Vector3f(0.0f, 1.0f, 0.0f)) * Math::Scale(finalScale);
			}
			///////////////////////////////////////

			Matrix4f viewProj = m_RenderSceneState.ActiveCamera.GetProjectionMatrix() * Math::Inverse(m_RenderSceneState.CameraTransform);

			// May be able to remove, its in the ubo
			billboardShader->SetMatrix4(Constants::Uniforms::ViewProj, viewProj);

			billboardShader->SetMatrix4(Constants::Uniforms::Transform, billboardTransform);
			billboardShader->SetFloat4(Constants::Uniforms::Color, billboard.Tint);
			billboardShader->SetInt(Constants::Uniforms::EntityID, entity);
			billboardShader->SetInt(Constants::Uniforms::Image, 0);

			RenderAction::SetTextureUnit(0, texture->GetID());

			Renderer3D::Submit(PrimitiveGenerator::CreateQuad(1.0f, 1.0f)->GetVertexArray());
		}

		m_HdrSceneBuffer->Unbind();

		RenderAction::UseDepthTest(false);
		RenderAction::UseBlending(false);
	}

	void RenderSystem::RenderWorldSpace2D(Scene* scene)
	{
		auto& registry = scene->GetRegistry();

		m_HdrSceneBuffer->Bind(); // Draw into the 3D scene

		// Enable depth testing so walls hide text, but disable writing so text doesn't cut holes
		RenderAction::UseDepthTest(true);
		RenderAction::UseDepthMask(false);
		RenderAction::UseBlending(true);

		Renderer2D::BeginFrame();

		// Draw World-Space Sprites
		for (EntityID entity : registry.ActiveQuery<SpriteComponent, TransformComponent>())
		{
			auto [sprite, transform] = registry.GetComponents<SpriteComponent, TransformComponent>(entity);
			if (sprite.TextureHandle == Constants::InvalidUUID)
			{
				Renderer2D::DrawQuad(transform.WorldTransform, sprite.Color);
			}
			else
			{
				auto textureAsset = Application::Instance().GetAssetManager().GetAsset<Texture2D>(sprite.TextureHandle);
				Renderer2D::DrawQuad(transform.WorldTransform, sprite.Color, textureAsset);
			}
		}

		// Draw World-Space Text
		for (EntityID entity : registry.ActiveQuery<TextComponent, TransformComponent>())
		{
			auto [textComp, transform] = registry.GetComponents<TextComponent, TransformComponent>(entity);
			if (!textComp.ScreenSpace && textComp.FontHandle != Constants::InvalidUUID && !textComp.Text.empty())
			{
				auto fontAsset = Application::Instance().GetAssetManager().GetAsset<Font>(textComp.FontHandle);
				if (fontAsset)
					Renderer2D::DrawString(textComp.Text, transform.WorldTransform, textComp.Color, fontAsset, entity, false);
			}
		}

		Renderer2D::EndFrame();

		RenderAction::UseDepthMask(true); // Always restore state!
		m_HdrSceneBuffer->Unbind();
	}

	void RenderSystem::RenderScreenSpaceUI(Scene* scene)
	{
		auto& registry = scene->GetRegistry();

		// Bind the final screen/editor output target
		RenderAction::SetFramebuffer(m_RenderSceneState.OutputFramebufferId);

		// UI ignores the 3D world completely
		RenderAction::UseDepthTest(false);
		RenderAction::UseDepthMask(false);
		RenderAction::UseBlending(true);

		// --- THE CAMERA SWAP ---
		// Create an Orthographic matrix perfectly sized to the viewport resolution
		float width = (float)m_RenderSceneState.ViewportDimensions.z;
		float height = (float)m_RenderSceneState.ViewportDimensions.w;

		// Note: Depending on your OpenGL setup, you may need to invert the top/bottom 
		// if your UI renders upside down.
		Matrix4f orthoProj = Math::Orthographic(0.0f, width, 0.0f, height, -1.0f, 1.0f);

		// Push the Ortho matrix to the shader's Camera UBO
		m_CameraUniformBuffer->SetData(&orthoProj, sizeof(Matrix4f));

		Renderer2D::BeginFrame();

		// Draw Screen-Space Sprites (e.g. Crosshairs, Minimaps)
		for (EntityID entity : registry.ActiveQuery<SpriteComponent, TransformComponent>())
		{
			auto [sprite, transform] = registry.GetComponents<SpriteComponent, TransformComponent>(entity);
			if (sprite.TextureHandle == Constants::InvalidUUID)
			{
				Renderer2D::DrawQuad(transform.WorldTransform, sprite.Color);
			}
			else
			{
				auto textureAsset = Application::Instance().GetAssetManager().GetAsset<Texture2D>(sprite.TextureHandle);
				Renderer2D::DrawQuad(transform.WorldTransform, sprite.Color, textureAsset);
			}
		}

		// Draw Screen-Space Text (e.g. Ammo, Health)
		for (EntityID entity : registry.ActiveQuery<TextComponent, TransformComponent>())
		{
			auto [textComp, transform] = registry.GetComponents<TextComponent, TransformComponent>(entity);
			if (textComp.ScreenSpace && textComp.FontHandle != Constants::InvalidUUID && !textComp.Text.empty())
			{
				auto fontAsset = Application::Instance().GetAssetManager().GetAsset<Font>(textComp.FontHandle);
				if (fontAsset)
					Renderer2D::DrawString(textComp.Text, transform.WorldTransform, textComp.Color, fontAsset, entity, true);
			}
		}

		Renderer2D::EndFrame();

		// --- RESTORE THE 3D CAMERA ---
		// Put the 3D ViewProjection matrix back in the UBO so the next frame starts correctly
		Matrix4f viewProjectionMat = m_RenderSceneState.ActiveCamera.GetProjectionMatrix() * Math::Inverse(m_RenderSceneState.CameraTransform);
		m_CameraUniformBuffer->SetData(&viewProjectionMat, sizeof(Matrix4f));

		RenderAction::UseDepthTest(true);
		RenderAction::UseDepthMask(true);
	}

	//void RenderSystem::HandlePostProcessing(Scene* scene)
	//{
	//	auto& registry = scene->GetRegistry();

	//	// TODO: Down the line, passing textures between shaders is slow so eventually want to 
	//	//  move to only a ping pong shader pass and a final composite shader.
	//	//  The ping pong pass will handle every post processing effect that needs the ping pong approach
	//	//  and the final composite shader will be an "uber shader" contain logic for every post processing effect
	//	//  that doesn't need the ping pong approach (i.e. outline, bloom, vignette, etc)
	//	//  This will drastically improve performance but will require a bit of an architectural change in how post processing passes are handled, 
	//	//  so for now we will just handle each post process effect as its own pass and optimize later

	//	RenderAction::UseDepthTest(false);

	//	// Ping-pong between two buffers: each pass reads from currentInput and writes to currentOutput
	//	SharedPtr<Framebuffer> currentInput = m_HdrSceneBuffer;
	//	SharedPtr<Framebuffer> currentOutput = m_PostProcessBufferA;

	//	// TODO: Need a more elegant way to handle these special VFX cases
	//	// Grab outline components for selected entities
	//	std::unordered_map<EntityID, OutlineComponent> outlinedEntityMap;
	//	View view = registry.Query<OutlineComponent>();
	//	for (EntityID entity : view)
	//	{
	//		auto [outline] = registry.GetComponents<OutlineComponent>(entity);
	//		outlinedEntityMap[entity] = outline;
	//	}

	//	// Pass over all post processing items
	//	for (auto& pass : m_PostProcessStack)
	//	{
	//		// TODO: Come up with solution for handling special cases
	//		if (auto outlinePass = DynamicPointerCast<OutlinePass>(pass))
	//		{
	//			if (pass->Enabled)
	//			{
	//				// Special case for outline pass since it needs the G-Buffer as well as the scene buffer
	//				for (const auto& [entityID, outline] : outlinedEntityMap)
	//				{
	//					outlinePass->SetGBuffer(m_GBuffer);
	//					outlinePass->SetHdrBuffer(m_HdrSceneBuffer);
	//					outlinePass->SetSelectedEntityID(entityID);
	//					outlinePass->SetOutlineColor(outline.Color);
	//					outlinePass->SetOutlineThickness(outline.Thickness);

	//					pass->Render(currentInput, currentOutput);
	//					currentInput = currentOutput;
	//					currentOutput = (currentOutput == m_PostProcessBufferA) ? m_PostProcessBufferB : m_PostProcessBufferA;
	//				}
	//			}
	//			
	//			continue;
	//		}

	//		if (pass->Enabled)
	//		{
	//			pass->Render(currentInput, currentOutput);
	//			currentInput = currentOutput;
	//			currentOutput = (currentOutput == m_PostProcessBufferA) ? m_PostProcessBufferB : m_PostProcessBufferA;
	//		}
	//	}

	//	RenderFinalComposite(currentInput);
	//}

	void RenderSystem::HandlePostProcessing(Scene* scene)
	{
		auto& registry = scene->GetRegistry();
		RenderAction::UseDepthTest(false);

		// Grab outline components for selected entities
		std::unordered_map<EntityID, OutlineComponent> outlinedEntityMap;
		View view = registry.ActiveQuery<OutlineComponent>();
		for (EntityID entity : view)
		{
			auto [outline] = registry.GetComponents<OutlineComponent>(entity);
			outlinedEntityMap[entity] = outline;
		}

		// --- PHASE 1: HDR PASSES (Bloom, Outlines) ---
		SharedPtr<Framebuffer> currentHdrInput = m_HdrSceneBuffer;
		SharedPtr<Framebuffer> currentHdrOutput = m_PostProcessBufferA;

		for (auto& pass : m_PostProcessStack)
		{
			if (pass->Enabled && pass->GetStage() == PostProcessStage::HDR)
			{
				// Special Case: Outline Pass needs specific buffers and loops per entity
				if (auto outlinePass = DynamicPointerCast<OutlinePass>(pass))
				{
					for (const auto& [entityID, outline] : outlinedEntityMap)
					{
						outlinePass->SetGBuffer(m_GBuffer);
						// MUST pass m_HdrSceneBuffer specifically so it can read Depth and IDs!
						outlinePass->SetHdrBuffer(m_HdrSceneBuffer);
						outlinePass->SetSelectedEntityID(entityID);
						outlinePass->SetOutlineColor(outline.Color);
						outlinePass->SetOutlineThickness(outline.Thickness);

						pass->Render(currentHdrInput, currentHdrOutput);

						// Ping Pong
						currentHdrInput = currentHdrOutput;
						currentHdrOutput = (currentHdrOutput == m_PostProcessBufferA) ? m_PostProcessBufferB : m_PostProcessBufferA;
					}
					continue; // Skip the standard render call below
				}

				// Standard HDR Passes (e.g. Bloom)
				pass->Render(currentHdrInput, currentHdrOutput);

				// Ping Pong
				currentHdrInput = currentHdrOutput;
				currentHdrOutput = (currentHdrOutput == m_PostProcessBufferA) ? m_PostProcessBufferB : m_PostProcessBufferA;
			}
		}

		// --- PHASE 2: TONE MAPPING (The Bridge) ---
		m_LdrBufferA->Bind();
		RenderAction::SetViewport(m_RenderSceneState.ViewportDimensions);
		RenderAction::Clear(Ember::RendererAPI::RenderBit::Color);

		auto finalShader = Application::Instance().GetAssetManager().GetAsset<Shader>(Constants::Assets::FinalCompositeShad);
		finalShader->Bind();
		finalShader->SetFloat(Constants::Uniforms::Exposure, 1.0f);

		finalShader->SetInt(Constants::Uniforms::Scene, 0);

		RenderAction::SetTextureUnit(0, currentHdrInput->GetColorAttachmentID(0));
		Renderer3D::Submit(m_ScreenQuad->GetVertexArray());
		m_LdrBufferA->Unbind();

		// --- PHASE 3: LDR PASSES (FXAA) ---
		SharedPtr<Framebuffer> currentLdrInput = m_LdrBufferA;
		SharedPtr<Framebuffer> currentLdrOutput = m_LdrBufferB;

		for (auto& pass : m_PostProcessStack)
		{
			if (pass->Enabled && pass->GetStage() == PostProcessStage::LDR)
			{
				pass->Render(currentLdrInput, currentLdrOutput);

				// Ping Pong
				currentLdrInput = currentLdrOutput;
				currentLdrOutput = (currentLdrOutput == m_LdrBufferA) ? m_LdrBufferB : m_LdrBufferA;
			}
		}

		// --- PHASE 4: BLIT TO SCREEN ---
		RenderAction::SetFramebuffer(m_RenderSceneState.OutputFramebufferId);
		RenderAction::SetViewport(m_RenderSceneState.ViewportDimensions);
		RenderAction::Clear(Ember::RendererAPI::RenderBit::Color | Ember::RendererAPI::RenderBit::Depth);

		auto blitShader = Application::Instance().GetAssetManager().GetAsset<Shader>(Constants::Assets::BlitShad);
		blitShader->Bind();

		blitShader->SetInt(Constants::Uniforms::Scene, 0);

		RenderAction::SetTextureUnit(0, currentLdrInput->GetColorAttachmentID(0));
		Renderer3D::Submit(m_ScreenQuad->GetVertexArray());
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

		// TODO: Move FinalComposite to a PostProcessPass and handle these uniforms there instead of hard coding them here
		// Set the exposure (hook this up to an ImGui slider later)
		finalShader->SetFloat(Constants::Uniforms::Exposure, 1.0f);
		finalShader->SetInt(Constants::Uniforms::Scene, 0);

		RenderAction::SetTextureUnit(0, outputBuffer->GetColorAttachmentID(0));

		Renderer3D::Submit(m_ScreenQuad->GetVertexArray());
	}

	void RenderSystem::RenderDebug(Scene* scene)
	{
		const auto& vertices = DebugRenderer::GetVertices();

		if (!vertices.empty())
		{
			auto physicsDebugShader = Application::Instance().GetAssetManager().GetAsset<Shader>(Constants::Assets::PhysicsDebugShadUUID);
			physicsDebugShader->Bind();

			uint32_t requiredSize = static_cast<uint32_t>(vertices.size() * sizeof(DebugVertex));
			if (requiredSize > m_PhysicsDebugLineVBO->GetSize())
			{
				// Immutable GPU storage cannot be resized — recreate the VBO and rebind it to the VAO
				m_PhysicsDebugLineVBO = VertexBuffer::Create(requiredSize);
				m_PhysicsDebugLineVBO->SetLayout({
					{ ShaderDataType::Float3, "v_Position" },
					{ ShaderDataType::Float4, "v_Color" }
					});
				m_PhysicsDebugLineVAO->AddVertexBuffer(m_PhysicsDebugLineVBO);
			}

			m_PhysicsDebugLineVBO->SetData(vertices.data(), requiredSize);
			m_PhysicsDebugLineVAO->Bind();
			RenderAction::DrawLines(m_PhysicsDebugLineVAO, static_cast<uint32_t>(vertices.size()));
		}

		DebugRenderer::Clear();
	}

	void RenderSystem::ResetRenderState()
	{
		RenderAction::UseDepthTest(true);
	}

	void RenderSystem::SortEntitiesByRenderQueue(Scene* scene)
	{
		auto& registry = scene->GetRegistry();

		auto sortLogic = [&](EntityID entity) {
			auto& material = registry.GetComponent<MaterialComponent>(entity);
			if (material.MaterialHandle == Constants::InvalidUUID)
				return;

			auto materialAsset = Application::Instance().GetAssetManager().GetAsset<MaterialBase>(material.MaterialHandle);
			switch (materialAsset->GetRenderQueue())
			{
			case RenderQueue::Opaque: m_RenderQueueBuckets.Opaque.push_back(entity); break;
			case RenderQueue::Forward: m_RenderQueueBuckets.Forward.push_back(entity); break;
			case RenderQueue::Transparent: m_RenderQueueBuckets.Transparent.push_back(entity); break;
			}
		};

		// Sort Static Meshes
		for (EntityID entity : registry.ActiveQuery<StaticMeshComponent, MaterialComponent, TransformComponent>()) {
			sortLogic(entity);
		}
		// Sort Skinned Meshes
		for (EntityID entity : registry.ActiveQuery<SkinnedMeshComponent, MaterialComponent, TransformComponent>()) {
			sortLogic(entity);
		}
	}

}