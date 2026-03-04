#include "ebpch.h"
#include "Renderer.h"
#include "RenderAction.h"

namespace Ember {

	ScopedPtr<Renderer::FrameData> Renderer::s_FrameData = ScopedPtr<Renderer::FrameData>::Create();

	void Renderer::BeginFrame(Camera& camera)
	{
		s_FrameData->ViewProjectionMatrix = camera.GetViewProjectionMatrix();
	}

	void Renderer::EndFrame()
	{

	}

	void Renderer::Submit(const SharedPtr<VertexArray>& vertexArray, const SharedPtr<Shader>& shader)
	{
		vertexArray->Bind();
		shader->Bind();
		shader->SetMatrix4("u_ViewProjection", s_FrameData->ViewProjectionMatrix);
		RenderAction::DrawInstanced(vertexArray);
	}

}