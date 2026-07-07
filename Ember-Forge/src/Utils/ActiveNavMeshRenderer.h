#pragma once

#include <Ember/ECS/Types.h>

namespace Ember {

	class Scene;

	class ActiveNavMeshRenderer
	{
	public:
		static void Draw(Scene* scene, EntityID selectedEntity);

		static void SetEnabled(bool enabled);
		static bool GetEnabled();
	};

}
