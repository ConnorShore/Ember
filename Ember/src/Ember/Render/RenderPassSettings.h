#pragma once

#include "Camera.h"
#include "ScreenSpaceRenderMode.h"
#include "Ember/Core/Filter.h"
#include "Ember/Core/Constants.h"
#include "Ember/ECS/Types.h"
#include "Ember/Math/Math.h"

#include <functional>

namespace Ember {

	class Scene;

	struct RenderPassSettings
	{
		Camera* ActiveCamera = nullptr;
		Matrix4f CameraTransform = Matrix4f(1.0f);
		Filter RenderMask = FilterPreset::All;
		Filter VolumeMask = FilterPreset::All;
		ScreenSpaceRenderMode ScreenSpaceMode = ScreenSpaceRenderMode::All;
		EntityID SelectedEntity = (EntityID)Constants::Entities::InvalidEntityID;
		std::function<void(Scene*, EntityID)> PreDebugDrawCallback = nullptr;
	};

}