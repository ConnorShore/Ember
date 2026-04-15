#include "ebpch.h"
#include "Raycast.h"

#include "Ember/Core/Application.h"
#include "Ember/ECS/System/PhysicsSystem.h"

namespace Ember {

	RaycastData Raycast::CastRay(const Vector3f& startPoint, const Vector3f& endPoint)
	{
		auto physicsSystem = Application::Instance().GetSystem<PhysicsSystem>();
		return physicsSystem->CastRay(startPoint, endPoint);
	}

}