#pragma once

#include "Ember/Core/Core.h"
#include "Ember/ECS/Component/Components.h"
#include "VertexArray.h"
#include "Shader.h"

namespace Ember {

	class Renderer3D
	{
	public:
		static void Init();
		static void Shutdown();

		static void BeginFrame(CameraComponent& camera, const Matrix4f& transform);
		static void EndFrame();

		static void Submit(const SharedPtr<VertexArray>& vertexArray, const SharedPtr<Shader>& shader, const Matrix4f& transform);
	};

}