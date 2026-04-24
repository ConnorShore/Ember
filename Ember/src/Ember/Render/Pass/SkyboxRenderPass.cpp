#include "ebpch.h"
#include "SkyboxRenderPass.h"

#include "Ember/Scene/Scene.h"
#include "Ember/Render/Skybox.h"
#include "Ember/Render/RenderAction.h"
#include "Ember/Render/Renderer3D.h"

namespace Ember {

	void SkyboxRenderPass::Init()
	{
		auto& assetManager = Application::Instance().GetAssetManager();
		m_SkyboxShader = Application::Instance().GetAssetManager().GetAsset<Shader>(Constants::Assets::SkyboxShad);
		m_CubeVAO = assetManager.GetAsset<Mesh>(Constants::Assets::CubeMeshUUID)->GetVertexArray();
	}

	void SkyboxRenderPass::Execute(RenderContext& context)
	{
		if (!context.ActiveSkybox || !context.ActiveSkybox->Enabled())
			return;

		m_FramebufferInputs["HDRScene"]->Bind();

		// Change depth function so the skybox (which will have a depth of 1.0) passes the depth test
		RenderAction::UseDepthFunction(RendererAPI::DepthFunction::LessEqual);
		RenderAction::UseDepthTest(true);
		RenderAction::UseFaceCulling(false); // Make sure we can see the inside of the cube

		auto skyboxShader = Application::Instance().GetAssetManager().GetAsset<Shader>(Constants::Assets::SkyboxShad);
		skyboxShader->Bind();

		skyboxShader->SetMatrix4(Constants::Uniforms::Projection, context.ActiveCamera->GetProjectionMatrix());
		skyboxShader->SetMatrix4(Constants::Uniforms::View, Math::Inverse(context.CameraTransform));

		// Bind the cubemap
		RenderAction::SetTextureUnit(0, context.ActiveSkybox->GetEnvironmentCubeMapID());
		skyboxShader->SetInt(Constants::Uniforms::EnvironmentMap, 0);

		skyboxShader->SetInt(Constants::Uniforms::EntityID, Constants::Entities::InvalidEntityID);

		Renderer3D::Submit(m_CubeVAO);

		// Reset depth function to default
		RenderAction::UseDepthFunction(RendererAPI::DepthFunction::Less);

		m_FramebufferInputs["HDRScene"]->Unbind();
	}

	void SkyboxRenderPass::OnViewportResize(uint32_t width, uint32_t height)
	{

	}

	void SkyboxRenderPass::Shutdown()
	{
	}

}