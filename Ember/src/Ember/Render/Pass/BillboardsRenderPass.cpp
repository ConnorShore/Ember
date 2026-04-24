#include "ebpch.h"
#include "BillboardsRenderPass.h"

#include "Ember/Scene/Scene.h"
#include "Ember/Render/Framebuffer.h"
#include "Ember/Render/RenderAction.h"
#include "Ember/Render/Renderer3D.h"
#include "Ember/Render/PrimitiveGenerator.h"

namespace Ember {

	void BillboardsRenderPass::Init()
	{
		m_BillboardShader = Application::Instance().GetAssetManager().GetAsset<Shader>(Constants::Assets::BillboardShad);
	}

	void BillboardsRenderPass::Execute(RenderContext& context)
	{
		auto& registry = context.ActiveScene->GetRegistry();

		RenderAction::UseBlending(true);
		RenderAction::UseDepthTest(false);

		m_FramebufferInputs["HDRScene"]->Bind();

		m_BillboardShader->Bind();

		auto& assetManager = Application::Instance().GetAssetManager();

		View view = registry.ActiveQuery<BillboardComponent, TransformComponent>();
		for (EntityID entity : view)
		{
			auto [billboard, transform] = registry.GetComponents<BillboardComponent, TransformComponent>(entity);
			if (context.ActiveScene->IsRuntime() && !billboard.RenderRuntime)
				continue;

			Matrix4f billboardTransform = CalculateBillboardTransform(context, transform, billboard);
			Matrix4f viewProj = context.ActiveCamera->GetProjectionMatrix() * Math::Inverse(context.CameraTransform);

			// TODO: May be able to remove, its in the ubo
			m_BillboardShader->SetMatrix4(Constants::Uniforms::ViewProj, viewProj);

			m_BillboardShader->SetMatrix4(Constants::Uniforms::Transform, billboardTransform);
			m_BillboardShader->SetFloat4(Constants::Uniforms::Color, billboard.Tint);
			m_BillboardShader->SetInt(Constants::Uniforms::EntityID, entity);
			m_BillboardShader->SetInt(Constants::Uniforms::Image, 0);
			
			// Set texture
			auto texture = assetManager.GetAsset<Texture2D>(billboard.TextureHandle);
			RenderAction::SetTextureUnit(0, texture->GetID());

			Renderer3D::Submit(PrimitiveGenerator::CreateQuad(1.0f, 1.0f)->GetVertexArray());
		}

		m_FramebufferInputs["HDRScene"]->Unbind();

		RenderAction::UseDepthTest(false);
		RenderAction::UseBlending(false);
	}

	void BillboardsRenderPass::OnViewportResize(uint32_t width, uint32_t height)
	{

	}

	void BillboardsRenderPass::Shutdown()
	{
	}

	Matrix4f BillboardsRenderPass::CalculateBillboardTransform(const RenderContext& context, const TransformComponent& transform, const BillboardComponent& billboard)
	{
		Vector3f worldPos, worldRot, worldScale;
		Math::DecomposeTransform(transform.WorldTransform, worldPos, worldRot, worldScale);

		// Find the billboards transform //
		Matrix4f cameraRotation = context.CameraTransform;
		cameraRotation[3] = Vector4f(0.0f, 0.0f, 0.0f, 1.0f); // Remove translation from camera transform

		// Scale billboard depending on if its static or not
		float distanceScale = billboard.Size;
		if (billboard.StaticSize)
		{
			float distance = Math::Length(worldPos - Vector3f(context.CameraTransform[3]));
			distanceScale = distance / 10.0f;
		}

		Vector3f finalScale = worldScale * distanceScale;

		Matrix4f billboardTransform;
		if (billboard.Spherical)
		{
			// Always faces the camera, but keeps its own position (using worldPos!)
			billboardTransform = Math::Translate(worldPos) * cameraRotation * Math::Scale(finalScale);
		}
		else
		{
			// Only want the camera's rotation on the Y axis for cylindrical billboards
			Vector3f cameraPos = Vector3f(context.CameraTransform[3]);
			Vector3f dirToCamera = cameraPos - worldPos;

			// Use atan2 to get the exact angle on the XZ plane
			float yaw = std::atan2(dirToCamera.x, dirToCamera.z);

			billboardTransform = Math::Translate(worldPos) * Math::Rotate(yaw, Vector3f(0.0f, 1.0f, 0.0f)) * Math::Scale(finalScale);
		}

		return billboardTransform;
	}

}