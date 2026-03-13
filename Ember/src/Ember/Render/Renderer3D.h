#pragma once

#include "Ember/Core/Core.h"
#include "Ember/ECS/Component/Components.h"
#include "VertexArray.h"
#include "Shader.h"

#include <tuple>

namespace Ember {

	class Renderer3D
	{
	public:
		static void Init();
		static void Shutdown();

		static void BeginFrame(CameraComponent& camera, const Matrix4f& transform);
		static void EndFrame();

		static void Submit(const SharedPtr<VertexArray>& vertexArray, const MaterialComponent& material, const Matrix4f& transform, 
			const std::array<std::tuple<PointLightComponent, TransformComponent>, 4>& lights);
	};

}