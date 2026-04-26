#include "ebpch.h"

#include "RenderSystem.h"
#include "PhysicsSystem.h"
#include "ParticleSystem.h"

#include "Ember/Core/Application.h"
#include "Ember/Scene/Scene.h"

#include "Ember/ECS/Component/Components.h"

#include "Ember/Render/RenderAction.h"
#include "Ember/Render/Renderer2D.h"
#include "Ember/Render/Renderer3D.h"
#include "Ember/Render/PrimitiveGenerator.h"
#include "Ember/Render/DebugRenderer.h"
#include "Ember/Render/RenderContext.h"
#include "Ember/Render/UniformBufferTypes.h"

#include "Ember/Render/VFX/BloomPass.h"
#include "Ember/Render/VFX/OutlinePass.h"
#include "Ember/Render/VFX/FXAAPass.h"
#include "Ember/Render/VFX/ColorGradePass.h"
#include "Ember/Render/VFX/ToneMapPass.h"

#include "Ember/Render/Pass/ShadowRenderPass.h"
#include "Ember/Render/Pass/DebugRenderPass.h"
#include "Ember/Render/Pass/BillboardsRenderPass.h"
#include "Ember/Render/Pass/TransparentEntitiesRenderPass.h"
#include "Ember/Render/Pass/SkyboxRenderPass.h"
#include "Ember/Render/Pass/ParticleRenderPass.h"
#include "Ember/Render/Pass/DeferredGeometryRenderPass.h"
#include "Ember/Render/Pass/DeferredLightingRenderPass.h"
#include "Ember/Render/Pass/ForwardEntitiesRenderPass.h"
#include "Ember/Render/Pass/WorldSpace2DRenderPass.h"
#include "Ember/Render/Pass/ScreenSpace2DRenderPass.h"
#include "Ember/Render/Pass/PostProcessRenderPass.h"
#include "Ember/Render/Pass/EditorGridRenderPass.h"
#include "Ember/Render/Pass/FinalBlitRenderPass.h"


namespace Ember {

	void RenderSystem::OnAttach()
	{
		Renderer2D::Init();
		Renderer3D::Init();

		// Uniform Buffer Objects at fixed binding points shared across all shaders
		m_CameraUniformBuffer = UniformBuffer::Create(sizeof(Matrix4f), 0);          // binding 0: ViewProjection
		m_ShadowUniformBuffer = UniformBuffer::Create(sizeof(ShadowDataBlock), 1);      // binding 1: DirLight + SpotLight VP
		m_LightUniformBuffer = UniformBuffer::Create(sizeof(LightDataBlock), 2);     // binding 2: All light data

		// TODO: Move this somewhere else - it doesn't need to be recreated every time the RenderSystem is attached to a scene,
		// but it does need to be recreated if the default skybox asset is changed or deleted
		m_Skybox = SharedPtr<Skybox>::Create(Constants::Assets::DefaultSkyboxUUID);
		RenderAction::UseCubeMapSeamless(true);

		// Framebuffer for color grading LUT baking
		Ember::FramebufferSpecification specs;
		specs.Width = 256;
		specs.Height = 16;
		specs.AttachmentSpecs = {
			Ember::FramebufferTextureFormat::RGB8
		};
		m_ColorGradeLUTBuffer = Framebuffer::Create(specs);

		// Initialize post process passes
		m_PostProcessStack["BloomPass"] = SharedPtr<BloomPass>::Create();
		m_PostProcessStack["OutlinePass"] = SharedPtr<OutlinePass>::Create();
		m_PostProcessStack["FXAAPass"] = SharedPtr<FXAAPass>::Create();
		m_PostProcessStack["ColorGradePass"] = SharedPtr<ColorGradePass>::Create();
		m_PostProcessStack["ToneMapPass"] = SharedPtr<ToneMapPass>::Create();
		for (auto& [_, pass] : m_PostProcessStack)
			pass->Init();

		// Initialize render passes
		m_RenderPasses["ShadowRenderPass"] = SharedPtr<ShadowRenderPass>::Create();
		m_RenderPasses["DeferredGeometryRenderPass"] = SharedPtr<DeferredGeometryRenderPass>::Create();
		m_RenderPasses["DeferredLightingRenderPass"] = SharedPtr<DeferredLightingRenderPass>::Create();
		m_RenderPasses["SkyboxRenderPass"] = SharedPtr<SkyboxRenderPass>::Create();
		m_RenderPasses["ForwardEntitiesRenderPass"] = SharedPtr<ForwardEntitiesRenderPass>::Create();
		m_RenderPasses["TransparentEntitiesRenderPass"] = SharedPtr<TransparentEntitiesRenderPass>::Create();
		m_RenderPasses["EditorGridRenderPass"] = SharedPtr<EditorGridRenderPass>::Create();
		m_RenderPasses["ParticleRenderPass"] = SharedPtr<ParticleRenderPass>::Create();
		m_RenderPasses["BillboardsRenderPass"] = SharedPtr<BillboardsRenderPass>::Create();
		m_RenderPasses["WorldSpace2DRenderPass"] = SharedPtr<WorldSpace2DRenderPass>::Create();
		m_RenderPasses["PostProcessRenderPass"] = SharedPtr<PostProcessRenderPass>::Create(m_PostProcessStack);
		m_RenderPasses["DebugRenderPass"] = SharedPtr<DebugRenderPass>::Create();
		m_RenderPasses["ScreenSpace2DRenderPass"] = SharedPtr<ScreenSpace2DRenderPass>::Create();
		m_RenderPasses["FinalBlitRenderPass"] = SharedPtr<FinalBlitRenderPass>::Create();

		for (auto& [_, pass] : m_RenderPasses)
			pass->Init();

		m_ScreenQuadVAO = PrimitiveGenerator::CreateQuad(2.0f, 2.0f)->GetVertexArray();

		m_RenderSceneState.Reset();
		EB_CORE_INFO("RenderSystem is attached!");
	}

	void RenderSystem::OnDetach()
	{
		for (auto& [_,pass] : m_RenderPasses)
			pass->Shutdown();

		Renderer2D::Shutdown();
		Renderer3D::Shutdown();
		EB_CORE_INFO("RenderSystem is detached!");
	}

	void RenderSystem::OnSceneAttach(Scene* scene)
	{
		if (scene->IsRuntime())
		{
			auto colorGradePass = StaticPointerCast<ColorGradePass>(GetPostProcessPass("ColorGradePass"));
			BakeColorGradeLUT(colorGradePass->Settings);
			colorGradePass->SetBakedLUT(m_ColorGradeLUTBuffer);
		}
	}

	void RenderSystem::ExecuteRenderPipeline(Scene* scene, bool isRuntime)
	{
		RenderAction::GetPreviousFramebuffer(&m_RenderSceneState.OutputFramebufferId);

		if (!m_RenderSceneState.IsCameraFound)
			return;

		// Set viewport dimensions before executing passes
		int dims[4] = { 0 };
		RenderAction::GetViewportDimensions(dims);
		m_RenderSceneState.ViewportDimensions = Vector4<int>(dims[0], dims[1], dims[2], dims[3]);

		// Setup render context
		RenderContext renderContext;
		renderContext.ActiveCamera = &m_RenderSceneState.ActiveCamera;
		renderContext.CameraTransform = m_RenderSceneState.CameraTransform;
		renderContext.ActiveScene = scene;
		renderContext.ActiveSkybox = m_Skybox;
		renderContext.LightUniformBuffer = m_LightUniformBuffer;
		renderContext.CameraUniformBuffer = m_CameraUniformBuffer;
		renderContext.ShadowUniformBuffer = m_ShadowUniformBuffer;
		renderContext.ViewportDimensions = m_RenderSceneState.ViewportDimensions;
		renderContext.IsRuntime = isRuntime;

		SortEntitiesByRenderQueue(scene);
		renderContext.RenderQueueBuckets = &m_RenderQueueBuckets;

		// --- Shadow pass ---
		auto shadowPass = StaticPointerCast<ShadowRenderPass>(GetRenderPass("ShadowRenderPass"));
		shadowPass->Execute(renderContext);

		// --- Deferred pipeline: geometry into GBuffer, then full-screen lighting resolve ---
		// Geometry
		auto deferredGeometryPass = StaticPointerCast<DeferredGeometryRenderPass>(GetRenderPass("DeferredGeometryRenderPass"));
		deferredGeometryPass->Execute(renderContext);

		// Lighting
		auto deferredLightingPass = StaticPointerCast<DeferredLightingRenderPass>(GetRenderPass("DeferredLightingRenderPass"));
		deferredLightingPass->SetTextureInput("AlbedoRoughness", deferredGeometryPass->GetTextureOutput("AlbedoRoughness"));
		deferredLightingPass->SetTextureInput("NormalMetallic", deferredGeometryPass->GetTextureOutput("NormalMetallic"));
		deferredLightingPass->SetTextureInput("PositionAO", deferredGeometryPass->GetTextureOutput("PositionAO"));
		deferredLightingPass->SetTextureInput("Emission", deferredGeometryPass->GetTextureOutput("Emission"));
		deferredLightingPass->SetTextureInput("DirectionalShadowMap", shadowPass->GetTextureOutput("DirectionalShadowMap"));
		deferredLightingPass->SetTextureInput("SpotShadowMap", shadowPass->GetTextureOutput("SpotShadowMap"));
		deferredLightingPass->Execute(renderContext);

		// Blit GBuffer depth into HDR buffer so forward objects are properly depth-tested
		RenderAction::CopyDepthBuffer(deferredGeometryPass->GetFramebufferOutput("GBuffer")->GetID(), deferredLightingPass->GetFramebufferOutput("HDRScene")->GetID(), m_RenderSceneState.ViewportDimensions);

		// --- Skybox ---
		auto skyboxPass = StaticPointerCast<SkyboxRenderPass>(GetRenderPass("SkyboxRenderPass"));
		skyboxPass->SetFramebufferInput("HDRScene", deferredLightingPass->GetFramebufferOutput("HDRScene"));
		skyboxPass->Execute(renderContext);

		// --- Forward pipeline: depth-tested draws on top of the deferred result ---
		auto forwardPass = StaticPointerCast<ForwardEntitiesRenderPass>(GetRenderPass("ForwardEntitiesRenderPass"));
		forwardPass->SetFramebufferInput("HDRScene", deferredLightingPass->GetFramebufferOutput("HDRScene"));
		forwardPass->Execute(renderContext);

		auto transparentPass = StaticPointerCast<TransparentEntitiesRenderPass>(GetRenderPass("TransparentEntitiesRenderPass"));
		transparentPass->Execute(renderContext);

		// --- Editor-only grid ---
		auto gridPass = StaticPointerCast<EditorGridRenderPass>(GetRenderPass("EditorGridRenderPass"));
		gridPass->SetFramebufferInput("HDRScene", deferredLightingPass->GetFramebufferOutput("HDRScene"));
		gridPass->Execute(renderContext);

		// Particles and billboards
		auto particlePass = StaticPointerCast<ParticleRenderPass>(GetRenderPass("ParticleRenderPass"));
		particlePass->SetFramebufferInput("HDRScene", deferredLightingPass->GetFramebufferOutput("HDRScene"));
		particlePass->Execute(renderContext);

		auto billboardPass = StaticPointerCast<BillboardsRenderPass>(GetRenderPass("BillboardsRenderPass"));
		billboardPass->SetFramebufferInput("HDRScene", deferredLightingPass->GetFramebufferOutput("HDRScene"));
		billboardPass->Execute(renderContext);

		// Draw World-Space 2D BEFORE Post-Processing
		auto worldSpace2DPass = StaticPointerCast<WorldSpace2DRenderPass>(GetRenderPass("WorldSpace2DRenderPass"));
		worldSpace2DPass->SetFramebufferInput("HDRScene", deferredLightingPass->GetFramebufferOutput("HDRScene"));
		worldSpace2DPass->Execute(renderContext);

		// Post Processing & Tone Mapping
		auto postProcessPass = StaticPointerCast<PostProcessRenderPass>(GetRenderPass("PostProcessRenderPass"));
		postProcessPass->SetFramebufferInput("GBuffer", deferredGeometryPass->GetFramebufferOutput("GBuffer"));
		postProcessPass->SetFramebufferInput("HDRScene", deferredLightingPass->GetFramebufferOutput("HDRScene"));
		postProcessPass->Execute(renderContext);

		// Final Blit to screen
		RenderAction::SetFramebuffer(m_RenderSceneState.OutputFramebufferId);

		auto finalBlitPass = StaticPointerCast<FinalBlitRenderPass>(GetRenderPass("FinalBlitRenderPass"));
		finalBlitPass->SetTextureInput("FinalScene", postProcessPass->GetTextureOutput("FinalScene"));
		finalBlitPass->Execute(renderContext);

		// Debug lines
		auto debugPass = StaticPointerCast<DebugRenderPass>(GetRenderPass("DebugRenderPass"));
		debugPass->Execute(renderContext);

		// Draw Screen-Space UI AFTER Final Composite
		RenderAction::SetFramebuffer(m_RenderSceneState.OutputFramebufferId);
		auto screenSpace2DPass = StaticPointerCast<ScreenSpace2DRenderPass>(GetRenderPass("ScreenSpace2DRenderPass"));
		screenSpace2DPass->Execute(renderContext);

		// Reset any modified render state so other systems aren't affected (like the Editor's Gizmo system)
		ResetRenderState();
	}

	void RenderSystem::OnUpdate(TimeStep delta, Scene* scene)
	{
		InitializeRenderState();
		SetSceneCamera(scene);

		if (m_RenderSceneState.IsCameraFound)
			ExecuteRenderPipeline(scene, true);
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
		ExecuteRenderPipeline(scene, false);
	}

	void RenderSystem::BakeColorGradeLUT(ColorGradeSettings& settings, const std::string& savePath /*= ""*/)
	{
		// Save the current scissor state, then disable it so we can draw to the whole 256x16 buffer
		// Prevents issues with ImGui's scissor test interfering with the baking process
		bool isScissorEnabled = RenderAction::IsScissorTestEnabled();
		RenderAction::UseScissorTest(false);

		m_ColorGradeLUTBuffer->Bind();

		RenderAction::SetViewport(0, 0, 256, 16);

		// (This ensures if the quad fails to draw, you get a black image, not a white one)
		RenderAction::SetClearColor({ 1.0f, 0.0f, 1.0f, 1.0f });
		RenderAction::Clear();

		auto& assetManager = Application::Instance().GetAssetManager();

		auto colorGradeShader = assetManager.GetAsset<Shader>(Constants::Assets::ColorGradeEditorShadUUID);
		colorGradeShader->Bind();

		colorGradeShader->SetInt(Constants::Uniforms::Scene, 0);
		auto neutralLUT = assetManager.GetAsset<Texture2D>(Constants::Assets::DefaultNeutralColorLUTUUID);
		RenderAction::SetTextureUnit(0, neutralLUT->GetID());

		auto colorGradePass = StaticPointerCast<ColorGradePass>(GetPostProcessPass("ColorGradePass"));
		auto inputLUT = colorGradePass->GetBaseBakedLUT();

		colorGradeShader->SetInt("u_BaseBakedLUT", 1);
		RenderAction::SetTextureUnit(1, inputLUT->GetID());

		colorGradeShader->SetFloat("u_Temperature", settings.Temperature);
		colorGradeShader->SetFloat("u_Tint", settings.Tint);
		colorGradeShader->SetFloat("u_Contrast", settings.Contrast);
		colorGradeShader->SetFloat("u_Saturation", settings.Saturation);
		colorGradeShader->SetFloat4("u_Lift", settings.Lift);
		colorGradeShader->SetFloat4("u_Gamma", settings.Gamma);
		colorGradeShader->SetFloat4("u_Gain", settings.Gain);

		Renderer3D::Submit(m_ScreenQuadVAO);

		const void* bakedData = m_ColorGradeLUTBuffer->ReadPixels(0, 0, 0, 256, 16);

		m_ColorGradeLUTBuffer->Unbind();

		// Restore previous scissor state
		if (isScissorEnabled)
			RenderAction::UseScissorTest(true);

		if (!savePath.empty())
		{
			stbi_flip_vertically_on_write(true);
			stbi_write_png(savePath.c_str(), 256, 16, 3, bakedData, 256 * 3);
			EB_CORE_INFO("Baked Color Grade LUT saved to {}", savePath);
		}

		free((void*)bakedData);
	}

	void RenderSystem::OnViewportResize(uint32_t width, uint32_t height)
	{
		for (auto& [_, pass] : m_PostProcessStack)
			pass->OnViewportResize(width, height);

		for (auto& [_, pass] : m_RenderPasses)
			pass->OnViewportResize(width, height);
	}

	// Reads the entity ID from the framebuffer at a pixel. Checks the forward buffer
	// first (drawn on top), falling back to the GBuffer's entity ID attachment.
	EntityID RenderSystem::GetEntityIDAtPixel(uint32_t x, uint32_t y)
	{
		// Check the Forward buffer first (since it is drawn on top of the world)
		auto hdrSceneBuffer = StaticPointerCast<DeferredLightingRenderPass>(GetRenderPass("DeferredLightingRenderPass"))->GetFramebufferOutput("HDRScene");
		
		hdrSceneBuffer->Bind();
		int forwardPixelData = hdrSceneBuffer->ReadPixel(2, x, y);
		hdrSceneBuffer->Unbind();

		if (forwardPixelData != Constants::Entities::InvalidEntityID)
			return (EntityID)forwardPixelData;

		// If the Forward buffer was empty, fallback to the Opaque G-Buffer!
		auto gBuffer = StaticPointerCast<DeferredGeometryRenderPass>(GetRenderPass("DeferredGeometryRenderPass"))->GetFramebufferOutput("GBuffer");
		
		gBuffer->Bind();
		int opaquePixelData = gBuffer->ReadPixel(4, x, y);
		gBuffer->Unbind();

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