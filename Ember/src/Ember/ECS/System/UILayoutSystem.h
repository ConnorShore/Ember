#pragma once

#include "System.h"
#include "Ember/ECS/Types.h"
#include "Ember/ECS/Registry.h"
#include "Ember/Math/Math.h"

namespace Ember {

	class Scene;

	class UILayoutSystem : public System
	{
	public:
		UILayoutSystem() = default;
		virtual ~UILayoutSystem() = default;

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate(TimeStep delta, Scene* scene) override;
		void OnViewportResize(Scene* scene, uint32_t width, uint32_t height);
	};

}