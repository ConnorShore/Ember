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
#include "Ember/Render/RenderQueueBuckets.h"
#include "Ember/Render/Pass/RenderPass.h"

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

		inline SharedPtr<Skybox> GetSkybox() const { return m_Skybox; }

		template<std::derived_from<PostProcessPass> T>
		inline SharedPtr<PostProcessPass> GetPostProcessPass() const
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
		void ResetRenderState();
		void SortEntitiesByRenderQueue(Scene* scene);

		template<std::derived_from<RenderPass> T>
		inline SharedPtr<RenderPass> GetRenderPass() const
		{
			for (const auto& pass : m_RenderPasses)
			{
				if (auto renderPass = DynamicPointerCast<T>(pass))
				{
					return renderPass;
				}
			}
			EB_CORE_ERROR("System of type {0} not found in RenderSystem!", typeid(T).name());
			return nullptr;
		}

	private:
		std::vector<SharedPtr<PostProcessPass>> m_PostProcessStack;

		SharedPtr<UniformBuffer> m_CameraUniformBuffer;
		SharedPtr<UniformBuffer> m_ShadowUniformBuffer;
		SharedPtr<UniformBuffer> m_LightUniformBuffer;

		// TODO: Make this a render graph
		std::vector<SharedPtr<RenderPass>> m_RenderPasses;

		// Skybox handler
		SharedPtr<Skybox> m_Skybox;

		RenderQueueBuckets m_RenderQueueBuckets;

		// TODO: See what can be removed from RenderSceneState
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
	};

}