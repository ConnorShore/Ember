#include "ebpch.h"
#include "Renderer3D.h"
#include "RenderAction.h"

namespace Ember {

	struct RendererData3D
	{
		Matrix4f ViewProjectionMatrix = Matrix4f(1.0f);
		Vector3f CameraPosition = Vector3f(0.0f);

		SharedPtr<Texture> WhiteTexture;
		SharedPtr<Shader> StandardGeoShader;
		SharedPtr<Shader> StandardLitShader;
	};
	static ScopedPtr<RendererData3D> s_RendererData;

	void Renderer3D::Init()
	{
		s_RendererData = ScopedPtr<RendererData3D>::Create();

		// White texture
		uint32_t whiteTextureData = 0xffffffff;
		s_RendererData->WhiteTexture = Texture::Create();
		s_RendererData->WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));

		// Inject Lighting macros to Standard Lit Shader here
		ShaderMacros lightMacros;
		lightMacros["MAX_LIGHTS"] = std::to_string(MAX_LIGHTS);
		lightMacros["DEFAULT_AMBIENT"] = std::to_string(DEFAULT_AMBIENT);

		// Standard Shaders
		s_RendererData->StandardGeoShader = Shader::Create("Ember/assets/shaders/StandardGeometry.glsl");
		s_RendererData->StandardLitShader = Shader::Create("Ember/assets/shaders/StandardLit.glsl", lightMacros);
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

		RenderAction::DrawIndexed(vertexArray);
	}

	void Renderer3D::Submit(const SharedPtr<VertexArray>& vertexArray)
	{
		RenderAction::DrawIndexed(vertexArray);
	}

	SharedPtr<Texture> Renderer3D::GetWhiteTexture()
	{
		return s_RendererData->WhiteTexture;
	}

	SharedPtr<Shader> Renderer3D::GetStandardGeometryShader()
	{
		return s_RendererData->StandardGeoShader;
	}

	SharedPtr<Shader> Renderer3D::GetStandardLitShader()
	{
		return s_RendererData->StandardLitShader;
	}

}