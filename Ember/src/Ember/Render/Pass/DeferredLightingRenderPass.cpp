#include "ebpch.h"
#include "DeferredLightingRenderPass.h"

#include "Ember/Scene/Scene.h"
#include "Ember/Render/Framebuffer.h"
#include "Ember/Render/RenderAction.h"
#include "Ember/Render/Renderer3D.h"
#include "Ember/Render/Skybox.h"
#include "Ember/Render/PrimitiveGenerator.h"

namespace Ember {

	void DeferredLightingRenderPass::Init()
	{
		Ember::FramebufferSpecification specs;
		specs.Width = 1;
		specs.Height = 1;
		specs.AttachmentSpecs = {
			Ember::FramebufferTextureFormat::RGBA16F,
			Ember::FramebufferTextureFormat::RGBA16F,
			Ember::FramebufferTextureFormat::RedInteger,
			Ember::FramebufferTextureFormat::Depth24Stencil8
		};
		m_HdrSceneBuffer = Framebuffer::Create(specs);

		m_ScreenQuad = PrimitiveGenerator::CreateQuad(2.0f, 2.0f);
	}

	void DeferredLightingRenderPass::Execute(RenderContext& context)
	{
		auto& registry = context.ActiveScene->GetRegistry();

		// TODO: Set this maybe when creating the context
		int dims[4] = { 0 };
		RenderAction::GetViewportDimensions(dims);
		context.ViewportDimensions = Vector4<int>(dims[0], dims[1], dims[2], dims[3]);

		RenderAction::UseDepthTest(false);
		RenderAction::UseFaceCulling(false);

		m_HdrSceneBuffer->Bind();
		RenderAction::SetViewport(context.ViewportDimensions);

		RenderAction::SetClearColor(Ember::Vector4f(0.0f, 0.0f, 0.0f, 1.0f));
		RenderAction::Clear(Ember::RendererAPI::RenderBit::Color);

		// clear entity id attachment
		int clearValue = Constants::Entities::InvalidEntityID;
		m_HdrSceneBuffer->ClearAttachment(2, clearValue);

		auto& assetManager = Application::Instance().GetAssetManager();
		auto litShader = assetManager.GetAsset<Shader>(Constants::Assets::StandardLitShad);

		litShader->Bind();
		litShader->SetFloat3(Constants::Uniforms::CameraPosition, context.CameraTransform[3]);

		// Extract the forward vector (-Z axis) from the Camera's World Transform
		Vector3f cameraForward = -Vector3f(context.CameraTransform[2]);
		litShader->SetFloat3(Constants::Uniforms::CameraForward, cameraForward); // Make sure you add this to your Constants::Uniforms!

		if (context.ActiveSkybox->Enabled())
			litShader->SetFloat(Constants::Uniforms::EnvironmentIntensity, context.ActiveSkybox->GetIntensity());
		else
			litShader->SetFloat(Constants::Uniforms::EnvironmentIntensity, 0.0f);

		litShader->SetInt(Constants::Uniforms::AlbedoRoughness, 0);
		litShader->SetInt(Constants::Uniforms::NormalMetallic, 1);
		litShader->SetInt(Constants::Uniforms::PositionAO, 2);
		litShader->SetInt(Constants::Uniforms::EmissionOut, 3);
		litShader->SetInt(Constants::Uniforms::DirectionShadowMap, 4);
		litShader->SetInt(Constants::Uniforms::SpotShadowMap, 5);
		litShader->SetInt(Constants::Uniforms::IrradianceMap, 6);
		litShader->SetInt(Constants::Uniforms::PrefilterMap, 7);
		litShader->SetInt(Constants::Uniforms::BRDFLUT, 8);

		RenderAction::SetTextureUnit(0, m_TextureInputs["AlbedoRoughness"]);
		RenderAction::SetTextureUnit(1, m_TextureInputs["NormalMetallic"]);
		RenderAction::SetTextureUnit(2, m_TextureInputs["PositionAO"]);
		RenderAction::SetTextureUnit(3, m_TextureInputs["Emission"]);
		RenderAction::SetTextureUnit(4, m_TextureInputs["DirectionalShadowMap"]);
		RenderAction::SetTextureUnit(5, m_TextureInputs["SpotShadowMap"]);
		RenderAction::SetTextureUnit(6, context.ActiveSkybox->GetIrradianceMapID());
		RenderAction::SetTextureUnit(7, context.ActiveSkybox->GetPrefilteredMapID());
		RenderAction::SetTextureUnit(8, context.ActiveSkybox->GetBRDFLUTID());

		LightDataBlock lightData = {};

		// Directional Lights
		auto& registry = context.ActiveScene->GetRegistry();
		RenderDirectionalLights(context, lightData, registry);
		RenderSpotLights(context, lightData, registry);
		RenderPointLights(context, lightData, registry);

		context.LightUniformBuffer->SetData(&lightData, sizeof(LightDataBlock), 0);
		Renderer3D::Submit(m_ScreenQuad->GetVertexArray());
	}

	void DeferredLightingRenderPass::Shutdown()
	{
	}

	void DeferredLightingRenderPass::RenderDirectionalLights(RenderContext& context, LightDataBlock& lightData, Registry& registry)
	{
		View view = registry.ActiveQuery<DirectionalLightComponent, TransformComponent>();
		for (EntityID entity : view)
		{
			if (lightData.ActiveDirectionalLights >= Constants::Renderer::MaxDirectionalLights)
				break;

			auto [light, transform] = registry.GetComponents<DirectionalLightComponent, TransformComponent>(entity);
			int i = lightData.ActiveDirectionalLights;

			lightData.DirectionalLights[i].Direction = transform.GetForward();
			lightData.DirectionalLights[i].Color = light.Color;
			lightData.DirectionalLights[i].Intensity = light.Intensity;

			lightData.ActiveDirectionalLights++;
		}
	}

	void DeferredLightingRenderPass::RenderPointLights(RenderContext& context, LightDataBlock& lightData, Registry& registry)
	{
		View view = registry.ActiveQuery<PointLightComponent, TransformComponent>();
		for (EntityID entity : view)
		{
			if (lightData.ActivePointLights >= Constants::Renderer::MaxPointLights)
				break;

			auto [light, transform] = registry.GetComponents<PointLightComponent, TransformComponent>(entity);
			int i = lightData.ActivePointLights;

			lightData.PointLights[i].Position = Vector3f(transform.WorldTransform[3]);
			lightData.PointLights[i].Color = light.Color;
			lightData.PointLights[i].Intensity = light.Intensity;
			lightData.PointLights[i].Radius = light.Radius;

			lightData.ActivePointLights++;
		}
	}

	void DeferredLightingRenderPass::RenderSpotLights(RenderContext& context, LightDataBlock& lightData, Registry& registry)
	{
		View view = registry.ActiveQuery<SpotLightComponent, TransformComponent>();
		for (EntityID entity : view)
		{
			if (lightData.ActiveSpotLights >= Constants::Renderer::MaxSpotLights)
				break;

			auto [light, transform] = registry.GetComponents<SpotLightComponent, TransformComponent>(entity);
			int i = lightData.ActiveSpotLights;

			lightData.SpotLights[i].Position = Vector3f(transform.WorldTransform[3]);
			lightData.SpotLights[i].Direction = transform.GetForward();
			lightData.SpotLights[i].Color = light.Color;
			lightData.SpotLights[i].Intensity = light.Intensity;
			lightData.SpotLights[i].CutOff = light.CutOff;
			lightData.SpotLights[i].OuterCutOff = light.OuterCutOff;

			lightData.ActiveSpotLights++;
		}
	}

}