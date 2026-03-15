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
		static constexpr unsigned int MAX_LIGHTS = 256;
		static constexpr float DEFAULT_AMBIENT = 0.03;

	public:
		static void Init();
		static void Shutdown();

		static void BeginFrame();
		static void EndFrame();

		static void Submit(const SharedPtr<VertexArray>& vertexArray, const MaterialComponent& material, const Matrix4f& transform);
		static void Submit(const SharedPtr<VertexArray>& vertexArray);

		static SharedPtr<Texture> GetWhiteTexture();
		static SharedPtr<Shader> GetStandardGeometryShader();
		static SharedPtr<Shader> GetStandardLitShader();
		static SharedPtr<Shader> GetStandardUnlitShader();
	};

}