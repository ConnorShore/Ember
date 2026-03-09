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

	void Renderer::DrawSprite(const SpriteComponent& sprite, const Matrix4f transform)
	{
		sprite.VertexArray->Bind();
		sprite.Shader->Bind();
		sprite.Shader->SetMatrix4("u_ViewProjection", s_FrameData->ViewProjectionMatrix);
		sprite.Shader->SetMatrix4("u_Model", transform);
		sprite.Shader->SetFloat4("u_Color", sprite.Color);
		RenderAction::DrawInstanced(sprite.VertexArray);
	}

}