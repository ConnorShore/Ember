#pragma once

#include "Ember/Scene/Scene.h"

#include "UniformBufferTypes.h"
#include "UniformBuffer.h"
#include "RenderQueueBuckets.h"

namespace Ember {

	class Scene;
	class Camera;
	class Skybox;

	struct RenderContext
	{
		Scene* ActiveScene;
		Camera* ActiveCamera;

		Matrix4f CameraTransform;
		Vector4<int> ViewportDimensions;

		SharedPtr<UniformBuffer> CameraUniformBuffer;
		SharedPtr<UniformBuffer> ShadowUniformBuffer;
		SharedPtr<UniformBuffer> LightUniformBuffer;

		SharedPtr<Skybox> ActiveSkybox;

		RenderQueueBuckets* RenderQueueBuckets;
	};

}