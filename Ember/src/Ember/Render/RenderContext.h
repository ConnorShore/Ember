#pragma once

#include "Ember/Scene/Scene.h"

#include "UniformBufferTypes.h"
#include "UniformBuffer.h"
#include "RenderQueueBuckets.h"

namespace Ember {

	class Scene;
	class Camera;

	struct RenderContext
	{
		Scene* ActiveScene;
		Camera* ActiveCamera;

		Matrix4f CameraTransform;

		SharedPtr<UniformBuffer> CameraUniformBuffer;
		SharedPtr<UniformBuffer> ShadowUniformBuffer;
		SharedPtr<UniformBuffer> LightUniformBuffer;

		RenderQueueBuckets* RenderQueueBuckets;
	};

}