#pragma once

#include "Ember/Core/Core.h"
#include "Ember/ECS/Component/Components.h"
#include "VertexArray.h"
#include "Shader.h"
#include "Texture.h"

#include <tuple>

namespace Ember {

	class Renderer3D
	{
	public:
		static void Init();
		static void Shutdown();

		static void BeginFrame();
		static void EndFrame();

		static void Submit(const SharedPtr<VertexArray>& vertexArray, const MaterialComponent& material, const Matrix4f& transform);
		static void Submit(const SharedPtr<VertexArray>& vertexArray);
	};

}