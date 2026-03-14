#include "ebpch.h"
#include "Renderer3D.h"
#include "RenderAction.h"

namespace Ember {

	struct RendererData3D
	{
		Matrix4f ViewProjectionMatrix = Matrix4f(1.0f);
		Vector3f CameraPosition = Vector3f(0.0f);
	};

	static ScopedPtr<RendererData3D> s_RendererData;
	static SharedPtr<Texture> s_WhiteTexture;

	void Renderer3D::Init()
	{
		s_RendererData = ScopedPtr<RendererData3D>::Create();

		uint32_t whiteTextureData = 0xffffffff;
		s_WhiteTexture = Texture::Create();
		s_WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));
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
		s_RendererData->CameraPosition = Vector3f(transform[3]);
	}

	void Renderer3D::EndFrame()
	{
		RenderAction::UseDepthTest(false);
	}

	void Renderer3D::Submit(const SharedPtr<VertexArray>& vertexArray, const MaterialComponent& material,  const Matrix4f& transform)
	{
		material.Material->Bind();

		// Required Uniforms
		material.Material->GetShader()->SetMatrix4("u_Transform", transform);
		material.Material->GetShader()->SetMatrix4("u_ViewProjection", s_RendererData->ViewProjectionMatrix);
		//material.Material->GetShader()->SetFloat3("u_CameraPos", s_RendererData->CameraPosition);

		RenderAction::DrawIndexed(vertexArray);
	}

	void Renderer3D::Submit(const SharedPtr<VertexArray>& vertexArray)
	{
		RenderAction::DrawIndexed(vertexArray);
	}

	//void Renderer3D::Submit(const SharedPtr<VertexArray>& vertexArray, const MaterialComponent& material, 
	//	const Matrix4f& transform, const std::array<std::tuple<PointLightComponent, TransformComponent>, 4>& lights)
	//{
	//	material.Material->Bind();

	//	// Required Uniforms
	//	material.Material->GetShader()->SetMatrix4("u_Transform", transform);
	//	material.Material->GetShader()->SetMatrix4("u_ViewProjection", s_RendererData->ViewProjectionMatrix);
	//	material.Material->GetShader()->SetFloat3("u_CameraPos", s_RendererData->CameraPosition);

	//	// Temporary lights before deferred shading
	//	for (unsigned int i = 0; i < lights.size(); i++) 
	//	{
	//		auto tup = lights[i];
	//		material.Material->GetShader()->SetFloat3(std::format("u_PointLights[{}].Position", i), std::get<1>(tup).Position);
	//		material.Material->GetShader()->SetFloat3(std::format("u_PointLights[{}].Color", i), std::get<0>(tup).Color);
	//		material.Material->GetShader()->SetFloat(std::format("u_PointLights[{}].Intensity", i), std::get<0>(tup).Intensity);
	//	}s

	//	RenderAction::DrawIndexed(vertexArray);
	//}

	Ember::SharedPtr<Ember::Texture> Renderer3D::GetWhiteTexture()
	{
		return s_WhiteTexture;
	}

}