#include "ebpch.h"
#include "Renderer3D.h"
#include "RenderAction.h"

namespace Ember {

	struct RendererData3D
	{
		Matrix4f ViewProjectionMatrix = Matrix4f(1.0f);
	};

	static ScopedPtr<RendererData3D> s_RendererData;

	void Renderer3D::Init()
	{
		s_RendererData = ScopedPtr<RendererData3D>::Create();
	}

	void Renderer3D::Shutdown()
	{
		s_RendererData.Reset();
	}

	void Renderer3D::BeginFrame(CameraComponent& camera, const Matrix4f& transform)
	{
		RenderAction::UseDepthTest(true);
		RenderAction::UseBlending(false);

		Matrix4f viewMatrix = Math::Inverse(transform);
		s_RendererData->ViewProjectionMatrix = camera.Camera.GetProjectionMatrix() * viewMatrix;
	}

	void Renderer3D::EndFrame()
	{
		RenderAction::UseDepthTest(false);
	}

	void Renderer3D::Submit(const SharedPtr<VertexArray>& vertexArray, const SharedPtr<Shader>& shader, const Matrix4f& transform)
	{
		shader->Bind();

		// u_Transform and u_ViewProjection are required for 3D Shaders
		//	TODO: Implement validation checking when loading shaders
		//	Need to find way to throw errors for 3d shaders vs 2d shaders
		//	(since batch rendering doesn't use u_Transform and bakes transform into vertex position)
		shader->SetMatrix4("u_Transform", transform);
		shader->SetMatrix4("u_ViewProjection", s_RendererData->ViewProjectionMatrix);

		RenderAction::DrawIndexed(vertexArray);
	}

}