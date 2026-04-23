#pragma once

#include "System.h"
#include "Ember/ECS/Registry.h"
#include "Ember/Render/Framebuffer.h"
#include "Ember/Render/UniformBuffer.h"
#include "Ember/Render/Shader.h"
#include "Ember/Render/Mesh.h"
#include "Ember/Render/CubeMap.h"
#include "Ember/Render/Skybox.h"
#include "Ember/ECS/Component/Components.h"
#include "Ember/Render/VFX/PostProcessPass.h"
#include "Ember/Render/Texture2DArray.h"

#include <vector>

namespace Ember {

	class RenderSystem : public System
	{
	public:
		RenderSystem() = default;
		virtual ~RenderSystem() = default;

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate(TimeStep delta, Scene* scene) override;
		void OnUpdate(TimeStep delta, Scene* scene, const Camera& camera, const Matrix4f& cameraTransform);

		void OnViewportResize(uint32_t width, uint32_t height);

		EntityID GetEntityIDAtPixel(uint32_t x, uint32_t y);

		SharedPtr<Skybox> GetSkybox() const { return m_Skybox; }

		template<std::derived_from<PostProcessPass> T>
		SharedPtr<PostProcessPass> GetPostProcessPass() const
		{
			for (const auto& pass : m_PostProcessStack)
			{
				if (auto postProcessPass = DynamicPointerCast<T>(pass))
				{
					return postProcessPass;
				}
			}

			EB_CORE_ERROR("System of type {0} not found in RenderSystem!", typeid(T).name());
			return nullptr;
		}

	private:
		void ExecuteRenderPipeline(Scene* scene, bool isRuntime);
		void InitializeRenderState();
		void SetSceneCamera(Scene* scene);
		void CreateShadowMaps(Scene* scene);
		void RenderGeometryForShadowMaps(Scene* scene, const Matrix4f& lightViewMatrix, const SharedPtr<Framebuffer>& shadowMapBuffer);
		void CreateDirectionalShadowMap(Scene* scene);
		void CreateSpotlightShadowMap(Scene* scene);
		void RenderDeferredGeometry(Scene* scene);
		void RenderDeferredLighting(Scene* scene);
		void RenderSkybox(Scene* scene);
		void RenderForwardEntities(Scene* scene);
		void RenderTransparentEntities(Scene* scene);
		void RenderInfiniteGrid();
		void RenderParticles(Scene* scene);
		void RenderBillboards(Scene* scene, bool isRuntime);
		void RenderWorldSpace2D(Scene* scene);
		void RenderScreenSpaceUI(Scene* scene);
		void HandlePostProcessing(Scene* scene);
		void RenderFinalComposite(const SharedPtr<Framebuffer>& outputBuffer);
		void RenderDebug(Scene* scene);
		void ResetRenderState();
		void SortEntitiesByRenderQueue(Scene* scene);

	private:
		//SharedPtr<Texture2DArray> m_TestTextureArray;
		//float m_SceneTime = 0.0f;
		SharedPtr<StaticMesh> m_ScreenQuad;

		SharedPtr<Framebuffer> m_GBuffer;
		SharedPtr<Framebuffer> m_DirectionalShadowMapBuffer;
		SharedPtr<Framebuffer> m_SpotShadowMapBuffer;
		SharedPtr<Framebuffer> m_LdrBufferA;	// Ping Pong buffer for LDR post-processing
		SharedPtr<Framebuffer> m_LdrBufferB;	// Ping Pong buffer for LDR post-processing

		std::vector<SharedPtr<PostProcessPass>> m_PostProcessStack;
		SharedPtr<Framebuffer> m_HdrSceneBuffer;
		SharedPtr<Framebuffer> m_PostProcessBufferA;
		SharedPtr<Framebuffer> m_PostProcessBufferB;

		SharedPtr<UniformBuffer> m_CameraUniformBuffer;
		SharedPtr<UniformBuffer> m_ShadowUniformBuffer;
		SharedPtr<UniformBuffer> m_LightUniformBuffer;

		// Physics Debug Data
		SharedPtr<VertexArray> m_PhysicsDebugLineVAO;
		SharedPtr<VertexBuffer> m_PhysicsDebugLineVBO;

		// Skybox handler
		SharedPtr<Skybox> m_Skybox;

		// Particles
		SharedPtr<VertexArray> m_ParticleVAO;
		SharedPtr<VertexBuffer> m_ParticleVBO;

		struct RenderQueueBuckets
		{
			std::vector<EntityID> Opaque;
			std::vector<EntityID> Forward;
			std::vector<EntityID> Transparent;

			void Clear()
			{
				Opaque.clear();
				Forward.clear();
				Transparent.clear();
			}
		} m_RenderQueueBuckets;

		struct RenderSceneState
		{
			Camera ActiveCamera;
			Matrix4f CameraTransform;

			std::vector<Matrix4f> DirectionalLightViewMatrices;
			std::vector<float> CascadeSplits;

			Matrix4f SpotLightViewMatrix;
			bool IsCameraFound;
			Vector4<int> ViewportDimensions;
			int OutputFramebufferId;

			void Reset()
			{
				ViewportDimensions = Vector4<int>(0);
				OutputFramebufferId = -1;
				DirectionalLightViewMatrices.clear();
				CascadeSplits.clear();

				// Reset light and cascades to 3 for now
				DirectionalLightViewMatrices.resize(3, Matrix4f(1.0f));
				CascadeSplits.resize(3, 0.0f);
			}

		} m_RenderSceneState;

		Scene* m_CurrentScene = nullptr;

		// These are the actual distance values from the camera. 
		// Cascade 0: CameraNear -> 15.0f
		// Cascade 1: 15.0f -> 50.0f
		// Cascade 2: 50.0f -> CameraFar
		std::vector<float> m_ShadowCascadeLevels = { 15.0f, 50.0f };

	};

}