#include "ebpch.h"
#include "Renderer3D.h"
#include "RenderAction.h"

namespace Ember {

	void Renderer3D::Init()
	{
	}

	void Renderer3D::Shutdown()
	{
	}

	void Renderer3D::BeginFrame()
	{
		RenderAction::UseDepthTest(true);
		RenderAction::UseBlending(false);
	}

	void Renderer3D::EndFrame()
	{
		RenderAction::UseDepthTest(false);
	}

	void Renderer3D::Submit(const SharedPtr<VertexArray>& vertexArray)
	{
		RenderAction::DrawIndexed(vertexArray);
	}

	void Renderer3D::Submit(const SharedPtr<VertexArray>& vertexArray, const MaterialComponent& material,  const Matrix4f& transform)
	{
		material.Material->Bind();

		// Required Uniforms
		material.Material->GetShader()->SetMatrix4(Constants::Uniforms::Transform, transform);

		RenderAction::DrawIndexed(vertexArray);
	}

}