#pragma once

#include "System.h"
#include "Ember/ECS/Registry.h"
#include "Ember/Render/Framebuffer.h"
#include "Ember/Render/Shader.h"
#include "Ember/Render/Mesh.h"
#include "Ember/ECS/Component/Components.h"

#include <vector>

namespace Ember {

	class RenderSystem : public System
	{
	public:
		RenderSystem() = default;
		virtual ~RenderSystem() = default;

		void OnAttach(Registry* registry) override;
		void OnDetach(Registry* registry) override;
		void OnUpdate(TimeStep delta, Registry* registry) override;

		void OnViewportResize(unsigned int width, unsigned int height);

	private:
		void InitializeRenderState();
		void SetSceneCamera(Registry* registry);
		void RenderDeferredGeometry(Registry* registry);
		void RenderDeferredLighting(Registry* registry);
		void RenderForwardEntities(Registry* registry);
		void RenderTransparentEntities(Registry* registry);
		void Render2DEntities(Registry* registry);
		void ResetRenderState();
		void SortEntitiesByRenderQueue(Registry* registry);

	private:
		SharedPtr<Framebuffer> m_GBuffer;
		SharedPtr<Mesh> m_ScreenQuad;

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
			CameraComponent ActiveCamera;
			Matrix4f CameraTransform;
			bool IsCameraFound;
			Vector4<int> ViewportDimensions;
			int OutputFramebufferId;

			void Reset()
			{
				ActiveCamera = CameraComponent();
				CameraTransform = Matrix4f(1.0f);
				IsCameraFound = false;
				ViewportDimensions = Vector4<int>(0);
				OutputFramebufferId = -1;
			}

		} m_RenderSceneState;
	};

}