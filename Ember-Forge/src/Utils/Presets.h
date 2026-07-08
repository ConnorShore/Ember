#pragma once

#include <Ember/Scene/Entity.h>
#include <Ember/Math/Math.h>

namespace Ember {

	class Presets
	{
	public:
		static Entity CreateFirstPersonCharacterController(const SharedPtr<Scene>& scene);
		static Entity CreateAICharacterController(const SharedPtr<Scene>& scene);
		static Entity CreateWaypoint(const SharedPtr<Scene>& scene);
		static Entity CreateNavigationGrid(const SharedPtr<Scene>& scene);
		static Entity CreateNavigationMesh(const SharedPtr<Scene>& scene);

		static Entity CreateCube(const SharedPtr<Scene>& scene);
		static Entity CreateQuad(const SharedPtr<Scene>& scene);
		static Entity CreateSphere(const SharedPtr<Scene>& scene);
		static Entity CreateCapsule(const SharedPtr<Scene>& scene);

		static Entity CreatePointLight(const SharedPtr<Scene>& scene);
		static Entity CreateDirectionalLight(const SharedPtr<Scene>& scene);
		static Entity CreateSpotLight(const SharedPtr<Scene>& scene);

		static Entity Create3DCamera(const SharedPtr<Scene>& scene, const Vector3f& position = Vector3f(0.0f), const Quaternion& orientation = Quaternion(1.0f, 0.0f, 0.0f, 0.0f));

		static Entity CreatePostProcessVolume(const SharedPtr<Scene>& scene);

		static Entity CreateCanvas(const SharedPtr<Scene>& scene);
		static Entity CreateUISprite(const SharedPtr<Scene>& scene);
		static Entity CreateUIText(const SharedPtr<Scene>& scene);

	};

}