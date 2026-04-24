#include "ebpch.h"
#include "EditorGridRenderPass.h"

#include "Ember/Core/Application.h"
#include "Ember/Render/Framebuffer.h"
#include "Ember/Render/RenderAction.h"
#include "Ember/Render/Renderer3D.h"
#include "Ember/Render/PrimitiveGenerator.h"

namespace Ember {

	void EditorGridRenderPass::Init()
	{
		m_GridShader = Application::Instance().GetAssetManager().GetAsset<Shader>(Constants::Assets::InfiniteGridShad);
		m_ScreenQuad = PrimitiveGenerator::CreateQuad(2.0f, 2.0f);
	}

	void EditorGridRenderPass::Execute(RenderContext& context)
	{
		m_FramebufferInputs["HDRScene"]->Bind();

		RenderAction::UseDepthTest(true);
		RenderAction::UseDepthMask(false);
		RenderAction::UseBlending(true);	// For fading away the grid lines as they get further from the camera

		m_GridShader->Bind();

		// The shader needs the inverse matrices to un-project the screen pixels back into 3D world space
		Matrix4f viewProj = context.ActiveCamera->GetProjectionMatrix() * Math::Inverse(context.CameraTransform);
		m_GridShader->SetMatrix4(Constants::Uniforms::ViewProj, viewProj);
		m_GridShader->SetMatrix4(Constants::Uniforms::InverseView, context.CameraTransform); // CameraTransform already is the inverse view
		m_GridShader->SetMatrix4(Constants::Uniforms::InverseProjection, Math::Inverse(context.ActiveCamera->GetProjectionMatrix()));
		m_GridShader->SetFloat3(Constants::Uniforms::CameraPosition, context.CameraTransform[3]);

		Renderer3D::Submit(m_ScreenQuad->GetVertexArray());

		RenderAction::UseBlending(false);
		RenderAction::UseDepthMask(true);

		m_FramebufferInputs["HDRScene"]->Unbind();
	}

	void EditorGridRenderPass::OnViewportResize(uint32_t width, uint32_t height)
	{

	}

	void EditorGridRenderPass::Shutdown()
	{
	}

}