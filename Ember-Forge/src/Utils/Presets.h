#pragma once

#include <Ember/Scene/Entity.h>
#include <Ember/Math/Math.h>

namespace Ember {

	class Presets
	{
	public:
		static Entity CreateCube(const SharedPtr<Scene>& scene);
		static Entity CreateQuad(const SharedPtr<Scene>& scene);
		static Entity CreateSphere(const SharedPtr<Scene>& scene);

		static Entity CreatePointLight(const SharedPtr<Scene>& scene);
		static Entity CreateDirectionalLight(const SharedPtr<Scene>& scene);
		static Entity CreateSpotLight(const SharedPtr<Scene>& scene);

		static Entity Create3DCamera(const SharedPtr<Scene>& scene, const Vector3f& position = Vector3f(0.0f), const Quaternion& orientation = Quaternion(1.0f, 0.0f, 0.0f, 0.0f));
	};

}