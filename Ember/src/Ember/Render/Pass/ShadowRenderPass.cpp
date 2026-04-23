#include "ebpch.h"
#include "ShadowRenderPass.h"

#include "Ember/Render/Framebuffer.h"
#include "Ember/Render/RenderAction.h"
#include "Ember/Render/Renderer3D.h"

namespace Ember {

	//////////////////////////////////////////////////////////////////////////
	// Utility Methods
	//////////////////////////////////////////////////////////////////////////

	static std::vector<Vector4f> GetFrustumCornersWorldSpace(const Matrix4f& viewProj)
	{
		const auto inv = Math::Inverse(viewProj);

		std::vector<Vector4f> frustumCorners;
		for (unsigned int x = 0; x < 2; ++x)
		{
			for (unsigned int y = 0; y < 2; ++y)
			{
				for (unsigned int z = 0; z < 2; ++z)
				{
					const Vector4f pt = inv * Vector4f(
						2.0f * x - 1.0f,
						2.0f * y - 1.0f,
						2.0f * z - 1.0f,
						1.0f);
					frustumCorners.push_back(pt / pt.w);
				}
			}
		}
		return frustumCorners;
	}

	static std::vector<Vector4f> GetFrustumCornersWorldSpace(const Matrix4f& proj, const Matrix4f& view)
	{
		Matrix4f viewProj = proj * view;
		return GetFrustumCornersWorldSpace(viewProj);
	}

	static Matrix4f GetLightSpaceMatrix(const float nearPlane, const float farPlane, Camera& camera, const Matrix4f& cameraTransform, const Vector3f& lightDir)
	{
		// 1. Create a projection matrix for ONLY this specific cascade slice
		Matrix4f proj = Math::Perspective(camera.GetPerspectiveProps().FieldOfView, camera.GetAspectRatio(), nearPlane, farPlane);
		Matrix4f view = Math::Inverse(cameraTransform);

		std::vector<Vector4f> corners = GetFrustumCornersWorldSpace(proj, view);

		// 2. Find the exact center of this cascade slice
		Vector3f center = Vector3f(0.0f, 0.0f, 0.0f);
		for (const auto& v : corners)
		{
			center += Vector3f(v.x, v.y, v.z);
		}
		center /= (float)corners.size();

		// 3. Build a Light View Matrix looking at the center of the cascade
		// We pull the light position back along the light direction.
		Matrix4f lightView = Math::LookAt(center - lightDir, center, Vector3f(0.0f, 1.0f, 0.0f));

		// 4. Find the min/max X, Y, Z of the corners in LIGHT SPACE
		// This tells us exactly how big our Orthographic bounding box needs to be
		float minX = std::numeric_limits<float>::max();
		float maxX = std::numeric_limits<float>::lowest();
		float minY = std::numeric_limits<float>::max();
		float maxY = std::numeric_limits<float>::lowest();
		float minZ = std::numeric_limits<float>::max();
		float maxZ = std::numeric_limits<float>::lowest();

		for (const auto& v : corners)
		{
			Vector4f trf = lightView * v;
			minX = std::min(minX, trf.x);
			maxX = std::max(maxX, trf.x);
			minY = std::min(minY, trf.y);
			maxY = std::max(maxY, trf.y);
			minZ = std::min(minZ, trf.z);
			maxZ = std::max(maxZ, trf.z);
		}

		// 5. THE Z-MULTIPLIER HACK (Crucial for shadows!)
		// If a tall building is behind the camera, it won't be inside the camera frustum,
		// but it STILL needs to cast a shadow into the frustum! We multiply the Z bounds 
		// heavily to catch geometry "behind" the light's view.
		constexpr float zMult = 10.0f;
		if (minZ < 0) minZ *= zMult;
		else minZ /= zMult;

		if (maxZ < 0) maxZ /= zMult;
		else maxZ *= zMult;

		// Add some padding to the bounds to reduce shadow acne and peter panning
		constexpr float padding = 2.0f;
		minX -= padding;
		maxX += padding;
		minY -= padding;
		maxY += padding;

		// 6. Build the final Orthographic projection
		Matrix4f lightProjection = Math::Orthographic(minX, maxX, minY, maxY, minZ, maxZ);

		return lightProjection * lightView;
	}

	//////////////////////////////////////////////////////////////////////////
	// ShadowRenderPass
	//////////////////////////////////////////////////////////////////////////

	void ShadowRenderPass::Init()
	{
		// Direction ShadowMap Buffer
		{
			Ember::FramebufferSpecification specs;
			specs.Width = m_ShadowMapResolution;
			specs.Height = m_ShadowMapResolution;
			specs.Layers = m_CascadeCount;
			specs.AttachmentSpecs = {
				Ember::FramebufferTextureFormat::Depth32
			};
			m_DirectionalShadowMapBuffer = Framebuffer::Create(specs);

			// Explicitly configure this specific generic buffer for shadows!
			m_DirectionalShadowMapBuffer->SetDepthBorderColor({ 1.0f, 1.0f, 1.0f, 1.0f });
		}
		m_TextureOutputs["DirectionalShadowMap"] = m_DirectionalShadowMapBuffer->GetDepthAttachmentID();

		// Spot ShadowMap Buffer
		{
			Ember::FramebufferSpecification specs;
			specs.Width = 2048;
			specs.Height = 2048;
			specs.Layers = 1;	// Only one spotlight shadow at a time for now, but we could easily expand this to support multiple spotlights in the future if needed
			specs.AttachmentSpecs = {
				Ember::FramebufferTextureFormat::Depth32
			};
			m_SpotShadowMapBuffer = Framebuffer::Create(specs);

			// Explicitly configure this specific generic buffer for shadows!
			m_SpotShadowMapBuffer->SetDepthBorderColor({ 1.0f, 1.0f, 1.0f, 1.0f });
		}
		m_TextureOutputs["SpotShadowMap"] = m_SpotShadowMapBuffer->GetDepthAttachmentID();

		auto& assetManager = Application::Instance().GetAssetManager();
		m_ShadowShader = assetManager.GetAsset<Shader>(Constants::Assets::StandardShadowShad);
		m_SkinnedShadowShader = assetManager.GetAsset<Shader>(Constants::Assets::StandardSkinnedShadowShad);
	}

	void ShadowRenderPass::Execute(RenderContext& context)
	{

	}

	void ShadowRenderPass::Shutdown()
	{
	}

	void ShadowRenderPass::CreateDirectionalShadowMap(RenderContext& context)
	{
		auto& registry = context.ActiveScene->GetRegistry();

		// Assuming you only have one directional light for shadows for now
		View lightView = registry.ActiveQuery<DirectionalLightComponent, TransformComponent>();
		if (lightView.Empty())
			return;

		EntityID lightEntity = lightView.Front();
		auto [light, transform] = registry.GetComponents<DirectionalLightComponent, TransformComponent>(lightEntity);
		Vector3f lightDirection = transform.GetForward();

		m_DirectionalShadowMapBuffer->Bind();
		RenderAction::SetViewport(0, 0, m_ShadowMapResolution, m_ShadowMapResolution);
		RenderAction::UseDepthTest(true);

		// The near and far plane of the main camera
		float cameraNear = context.ActiveCamera->GetPerspectiveProps().NearClip;
		float cameraFar = context.ActiveCamera->GetPerspectiveProps().FarClip;

		ShadowDataBlock shadowData = {};

		// We have 3 layers.
		for (uint32_t i = 0; i < m_CascadeCount; ++i)
		{
			// Figure out the near/far planes for THIS specific cascade
			float cascadeNear = (i == 0) ? cameraNear : m_ShadowCascadeLevels[i - 1] - m_BlendOverlap;
			float cascadeFar = (i == m_CascadeCount - 1) ? cameraFar : m_ShadowCascadeLevels[i] + m_BlendOverlap;

			Matrix4f lightSpaceMat = GetLightSpaceMatrix(
				cascadeNear, cascadeFar, // Use the expanded planes!
				*context.ActiveCamera,
				context.CameraTransform,
				lightDirection
			);

			// Save it so we can upload it to the UBO later
			shadowData.DirectionalShadowMatrices[i] = lightSpaceMat;
			shadowData.CascadeSplits[i] = (i == m_CascadeCount - 1) ? cameraFar : m_ShadowCascadeLevels[i];

			// IMPORTANT: Still store the REAL splits in the UBO so the shader 
			// knows where the mathematical center of the blend is!
			shadowData.DirectionalShadowMatrices[i] = lightSpaceMat;
			shadowData.CascadeSplits[i] = (i == m_CascadeCount - 1) ? cameraFar : m_ShadowCascadeLevels[i];

			// Route the Framebuffer to write to THIS specific layer of the array
			m_DirectionalShadowMapBuffer->AttachDepthTextureLayer(m_DirectionalShadowMapBuffer->GetDepthAttachmentID(), 0, i);

			// Clear ONLY this layer!
			RenderAction::Clear(Ember::RendererAPI::RenderBit::Depth);

			// Render the scene from this cascade's perspective
			RenderGeometryForShadowMaps(context, lightSpaceMat, m_DirectionalShadowMapBuffer);
		}

		context.ShadowUniformBuffer->SetData(&shadowData, sizeof(ShadowDataBlock), 0);
		m_DirectionalShadowMapBuffer->Unbind();
	}

	void ShadowRenderPass::CreateSpotlightShadowMap(RenderContext& context)
	{
		auto& registry = context.ActiveScene->GetRegistry();

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

			// TODO: Move spotlight to use cascade shadow maps in the future as well, but for now we will just use a single perspective projection for the spotlight shadow map
			Matrix4f lightProjection = Math::Perspective(Math::Degrees(light.OuterCutOffAngle) * 2.0f, 1.0f, 1.0f, 100.0f);
			Vector3f worldPos = Vector3f(transform.WorldTransform[3]);
			Vector3f target = lightDirection + worldPos;	// Look in the direction of the spotlight
			Vector3f eye = worldPos;
			Vector3f up = Vector3f(0.0f, 1.0f, 0.0f);
			Matrix4f lightView = Math::LookAt(eye, target, up);
			m_SpotLightViewMatrix = lightProjection * lightView;

			// Set uniform buffer for spotlight (offset -> 3 mat4)
			context.ShadowUniformBuffer->SetData(
				&m_SpotLightViewMatrix,
				sizeof(Matrix4f),
				sizeof(Matrix4f) * 3
			);

			index++;
		}

		RenderGeometryForShadowMaps(context, m_SpotLightViewMatrix, m_SpotShadowMapBuffer);
	}

	void ShadowRenderPass::RenderGeometryForShadowMaps(RenderContext& context, const Matrix4f& lightViewMatrix, const SharedPtr<Framebuffer>& shadowMapBuffer)
	{
		auto& registry = context.ActiveScene->GetRegistry();
		auto& assetManager = Application::Instance().GetAssetManager();

		shadowMapBuffer->Bind();

		RenderAction::SetViewport(0, 0, shadowMapBuffer->GetSpecification().Width, shadowMapBuffer->GetSpecification().Height);
		RenderAction::Clear(Ember::RendererAPI::RenderBit::Depth);
		RenderAction::UseDepthTest(true);

		// Split entities  so can bind each shader 1 time
		int size = (int)context.RenderQueueBuckets->Opaque.size();
		std::vector<EntityID> splitEntities(size);

		int staticCount = 0;
		int skinnedCount = 0;
		for (int i = 0; i < size; i++)
		{
			EntityID entity = context.RenderQueueBuckets->Opaque[i];
			if (registry.ContainsComponent<SkinnedMeshComponent>(entity))
				splitEntities[size - 1 - skinnedCount++] = entity; // Add to end of list
			else if (registry.ContainsComponent<StaticMeshComponent>(entity))
				splitEntities[staticCount++] = entity; // Keep at beginning of list
		}


		Renderer3D::BeginFrame();

		// Render static meshes with the static shadow shader
		m_ShadowShader->Bind();
		m_ShadowShader->SetMatrix4(Constants::Uniforms::LightViewMatrix, lightViewMatrix);

		for (int i = 0; i < staticCount; i++)
		{
			EntityID entity = splitEntities[i];
			auto [transform] = registry.GetComponents<TransformComponent>(entity);
			m_ShadowShader->SetMatrix4(Constants::Uniforms::Transform, transform.WorldTransform);
			if (registry.ContainsComponent<StaticMeshComponent>(entity))
			{
				auto& mesh = registry.GetComponent<StaticMeshComponent>(entity);
				auto meshAsset = assetManager.GetAsset<Mesh>(mesh.MeshHandle);
				Renderer3D::Submit(meshAsset->GetVertexArray());
			}
		}

		// Render skinned meshes with the skinned shadow shader
		m_SkinnedShadowShader->Bind();
		m_SkinnedShadowShader->SetMatrix4(Constants::Uniforms::LightViewMatrix, lightViewMatrix);

		for (int i = size - skinnedCount; i < size; i++)
		{
			EntityID entity = splitEntities[i];
			auto [transform] = registry.GetComponents<TransformComponent>(entity);
			m_SkinnedShadowShader->SetMatrix4(Constants::Uniforms::Transform, transform.WorldTransform);
			if (registry.ContainsComponent<SkinnedMeshComponent>(entity))
			{
				auto& mesh = registry.GetComponent<SkinnedMeshComponent>(entity);
				auto meshAsset = assetManager.GetAsset<Mesh>(mesh.MeshHandle);
				if (mesh.AnimatorEntityHandle != Constants::InvalidUUID && context.ActiveScene)
				{
					Entity animatorEntity = context.ActiveScene->GetEntity(mesh.AnimatorEntityHandle);
					if (animatorEntity.GetEntityHandle() != Constants::Entities::InvalidEntityID)
					{
						auto& animator = registry.GetComponent<AnimatorComponent>(animatorEntity.GetEntityHandle());
						m_SkinnedShadowShader->SetMatrix4Array(Constants::Uniforms::BoneMatrices, animator.BoneMatrices.data(), static_cast<uint32_t>(animator.BoneMatrices.size()));
					}
				}
				Renderer3D::Submit(meshAsset->GetVertexArray());
			}
		}

		Renderer3D::EndFrame();
	}

}