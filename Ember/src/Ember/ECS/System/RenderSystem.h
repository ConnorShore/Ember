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
#include "Ember/Render/Frustum.h"

#include <vector>
#include <map>

namespace Ember {

	class RenderSystem : public System
	{
	public:
		RenderSystem() = default;
		virtual ~RenderSystem() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		virtual void OnSceneAttach(Scene* scene) override;

		virtual void OnUpdate(TimeStep delta, Scene* scene) override;
		void OnUpdate(TimeStep delta, Scene* scene, const Camera& camera, const Matrix4f& cameraTransform);

		void BakeColorGradeLUT(ColorGradeSettings& settings, const std::string& savePath = "");

		void OnViewportResize(uint32_t width, uint32_t height);

		EntityID GetEntityIDAtPixel(uint32_t x, uint32_t y);

		inline SharedPtr<Skybox> GetSkybox() const { return m_Skybox; }

		inline SharedPtr<PostProcessPass> GetPostProcessPass(const std::string& name) const
		{
			EB_CORE_ASSERT(m_PostProcessStack.find(name) != m_PostProcessStack.end(), "Post process pass with name {} not found!", name);
			return m_PostProcessStack.at(name);
		}

		inline SharedPtr<RenderPass> GetRenderPass(const std::string& name) const
		{
			EB_CORE_ASSERT(m_RenderPasses.find(name) != m_RenderPasses.end(), "Render pass with name {} not found!", name);
			return m_RenderPasses.at(name);
		}

		void SetGlobalPostProcessVolumeSettings(const PostProcessVolumeSettings& settings) { m_GlobalVolumeSettings = settings; }

	private:
		void ExecuteRenderPipeline(Scene* scene, bool isRuntime);
		void InitializeRenderState();
		void SetSceneCamera(Scene* scene);
		void ResetRenderState();
		void StoreRenderableEntities(Scene* scene);
		void SortEntitiesByRenderQueue(Scene* scene);
		void SetFinalPostProcessSettings(Scene* scene);
		void ApplyPostProcessSettings();

	private:
		// TODO: Make this a render graph
		std::map<std::string, SharedPtr<RenderPass>> m_RenderPasses;
		std::map<std::string, SharedPtr<PostProcessPass>> m_PostProcessStack;

		SharedPtr<UniformBuffer> m_CameraUniformBuffer;
		SharedPtr<UniformBuffer> m_ShadowUniformBuffer;
		SharedPtr<UniformBuffer> m_LightUniformBuffer;

		SharedPtr<Framebuffer> m_ColorGradeLUTBuffer;

		RenderQueueBuckets m_RenderQueueBuckets;

		std::vector<std::pair<EntityID, AABB>> m_ActiveRenderableEntities;

		// Skybox handler
		SharedPtr<Skybox> m_Skybox;

		SharedPtr<VertexArray> m_ScreenQuadVAO;

		PostProcessVolumeSettings m_GlobalVolumeSettings;	// Baseline settings that all volumes will blend on top of

		struct RenderSceneState
		{
			Camera ActiveCamera;
			Matrix4f CameraTransform;
			Matrix4f CameraViewProjection;
			bool IsCameraFound;

			Vector4<int> ViewportDimensions;
			int OutputFramebufferId;

			PostProcessVolumeSettings FinalPostProcessVolumeSettings;

			void Reset()
			{
				ViewportDimensions = Vector4<int>(0);
				OutputFramebufferId = -1;
			}

		} m_RenderSceneState;
	};

}