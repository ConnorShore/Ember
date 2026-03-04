#include "ebpch.h"
#include "Renderer.h"
#include "RenderAction.h"

namespace Ember {

	void Renderer::BeginFrame()
	{

	}

	void Renderer::EndFrame()
	{

	}

	void Renderer::Submit(const SharedPtr<VertexArray>& vertexArray, const SharedPtr<Shader>& shader)
	{
		vertexArray->Bind();
		shader->Bind();
		RenderAction::DrawInstanced(vertexArray);
	}

}