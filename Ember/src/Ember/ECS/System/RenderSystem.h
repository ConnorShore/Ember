#pragma once

#include "System.h"
#include "Ember/ECS/Registry.h"
#include "Ember/Render/Framebuffer.h"
#include "Ember/Render/UniformBuffer.h"
#include "Ember/Render/Shader.h"
#include "Ember/Render/Mesh.h"
#include "Ember/Render/CubeMap.h"
#include "Ember/ECS/Component/Components.h"
#include "Ember/Render/VFX/PostProcessPass.h"

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
		void ExecuteRenderPipeline(Registry& registry, bool renderInfiniteGrid);
		void InitializeRenderState();
		void SetSceneCamera(Registry& registry);
		void CreateShadowMaps(Registry& registry);
		void RenderGeometryForShadowMaps(Registry& registry, const Matrix4f& lightViewMatrix, const SharedPtr<Framebuffer>& shadowMapBuffer);
		void CreateDirectionalShadowMap(Registry& registry);
		void CreateSpotlightShadowMap(Registry& registry);
		void RenderDeferredGeometry(Registry& registry);
		void RenderDeferredLighting(Registry& registry);
		void RenderForwardEntities(Registry& registry);
		void RenderTransparentEntities(Registry& registry);
		void RenderInfiniteGrid();
		void RenderBillboards(Registry& registry);
		void Render2DEntities(Registry& registry);
		void HandlePostProcessing(Registry& registry);
		void RenderFinalComposite(const SharedPtr<Framebuffer>& outputBuffer);
		void ResetRenderState();
		void SortEntitiesByRenderQueue(Registry& registry);

	private:
		SharedPtr<Mesh> m_ScreenQuad;

		SharedPtr<Framebuffer> m_GBuffer;
		SharedPtr<Framebuffer> m_DirectionalShadowMapBuffer;
		SharedPtr<Framebuffer> m_SpotShadowMapBuffer;

		std::vector<SharedPtr<PostProcessPass>> m_PostProcessStack;
		SharedPtr<Framebuffer> m_HdrSceneBuffer;
		SharedPtr<Framebuffer> m_PostProcessBufferA;
		SharedPtr<Framebuffer> m_PostProcessBufferB;

		SharedPtr<UniformBuffer> m_CameraUniformBuffer;
		SharedPtr<UniformBuffer> m_ShadowUniformBuffer;
		SharedPtr<UniformBuffer> m_LightUniformBuffer;

		//skybox testing
		SharedPtr<Texture2D> m_SkyboxTexture;
		SharedPtr<Framebuffer> m_SkyboxBuffer;
		SharedPtr<CubeMap> m_EnvironmentCubeMap;
		SharedPtr<Mesh> m_SkyboxCube;

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
			Matrix4f DirectionalLightViewMatrix;
			Matrix4f SpotLightViewMatrix;
			bool IsCameraFound;
			Vector4<int> ViewportDimensions;
			int OutputFramebufferId;

			void Reset()
			{
				ViewportDimensions = Vector4<int>(0);
				OutputFramebufferId = -1;
			}

		} m_RenderSceneState;
	};

}