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
#include "Ember/Render/RenderPassSettings.h"

#include <functional>
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
		void OnUpdate(TimeStep delta, Scene* scene, const RenderPassSettings& settings);

		void BakeColorGradeLUT(ColorGradeSettings& settings, const std::string& savePath = "");

		void OnViewportResize(uint32_t width, uint32_t height);
		const Vector2f& GetViewportSize() const { return m_RenderSceneState.ActiveCamera.GetViewportSize(); }

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

		// --- Golden-image smoke-test helpers (operate on the default framebuffer / back buffer) ---
		// Writes the current back buffer to a PNG (upright, top-left origin). Returns false on failure.
		static bool CaptureBackbufferToPNG(uint32_t width, uint32_t height, const std::string& outPath);
		// Compares the current back buffer against a reference PNG. Passes when the fraction of pixels
		// whose any channel differs by more than channelTolerance (0-255) is <= maxDiffFraction. Logs
		// PASS/FAIL with the measured difference.
		static bool CompareBackbufferToReference(uint32_t width, uint32_t height, const std::string& refPath,
			int channelTolerance, double maxDiffFraction);

		// Coarse back-buffer summary, so a test can assert a frame was actually drawn - not black, not
		// blown out, carrying real tonal variation - without needing a committed reference image.
		struct BackbufferStats
		{
			bool Valid = false;         // false if the read failed (zero-sized viewport)
			double MeanLuminance = 0.0; // 0..1
			double BlackFraction = 0.0; // fraction of pixels with RGB all <= 2
			double WhiteFraction = 0.0; // fraction of pixels with RGB all >= 253
			double StdDevLuminance = 0.0;
		};
		static BackbufferStats ComputeBackbufferStats(uint32_t width, uint32_t height);

	private:
		void ExecuteRenderPipeline(Scene* scene, bool isRuntime);
		void InitializeRenderState();
		void SetSceneCamera(Scene* scene);
		void ResetRenderState();
		void StoreRenderableEntities(Scene* scene);
		void SortEntitiesByRenderQueue(Scene* scene);
		void SetFinalPostProcessSettings(Scene* scene);
		void ApplyPostProcessSettings();
		void BakeColorGradeLUTIfDirty(ColorGradeSettings& settings);

	private:
		// TODO: Make this a render graph
		std::map<std::string, SharedPtr<RenderPass>> m_RenderPasses;
		std::map<std::string, SharedPtr<PostProcessPass>> m_PostProcessStack;

		SharedPtr<UniformBuffer> m_CameraUniformBuffer;
		SharedPtr<UniformBuffer> m_ShadowUniformBuffer;
		SharedPtr<UniformBuffer> m_LightUniformBuffer;

		SharedPtr<Framebuffer> m_ColorGradeLUTBuffer;
		// Settings used for the most recent LUT bake; drives the dirty check in BakeColorGradeLUTIfDirty.
		ColorGradeSettings m_LastBakedColorGradeSettings;
		bool m_HasBakedColorGradeLUT = false;

		RenderQueueBuckets m_RenderQueueBuckets;

		std::vector<std::pair<EntityID, AABB>> m_ActiveRenderableEntities;

		// Skybox handler
		SharedPtr<Skybox> m_Skybox;

		SharedPtr<VertexArray> m_ScreenQuadVAO;

		PostProcessVolumeSettings m_GlobalVolumeSettings;	// Baseline settings that all volumes will blend on top of

		struct RenderSceneState
		{
			Camera ActiveCamera;
			Filter ActiveRenderMask = FilterPreset::All;
			Filter ActiveVolumeMask = FilterPreset::All;
			Matrix4f CameraTransform = Matrix4f(1.0f);
			Matrix4f CameraViewProjection = Matrix4f(1.0f);
			bool IsCameraFound = false;

			Vector4<int> ViewportDimensions = Vector4<int>(0);
			int OutputFramebufferId = -1;

			PostProcessVolumeSettings FinalPostProcessVolumeSettings;

			bool DrawHUD = true;
			EntityID SelectedEntity = (EntityID)Constants::Entities::InvalidEntityID;
			std::function<void(Scene*, EntityID)> PreDebugDrawCallback = nullptr;

			void Reset()
			{
				ActiveRenderMask = FilterPreset::All;
				ActiveVolumeMask = FilterPreset::All;
				CameraTransform = Matrix4f(1.0f);
				CameraViewProjection = Matrix4f(1.0f);
				IsCameraFound = false;
				ViewportDimensions = Vector4<int>(0);
				OutputFramebufferId = -1;
				DrawHUD = true;
				SelectedEntity = (EntityID)Constants::Entities::InvalidEntityID;
				PreDebugDrawCallback = nullptr;
			}

		} m_RenderSceneState;
	};

}