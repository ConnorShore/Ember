#include "ebpch.h"
#include "Skybox.h"
#include "RenderAction.h"
#include "Renderer3D.h"

#include "Ember/Core/Application.h"

namespace Ember {

	Skybox::Skybox()
	{
		m_CaptureProjection = Math::Perspective(90.0f, 1.0f, 0.1f, 1000.0f);
		m_CaptureViewMats = { 
			Math::LookAt({ 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f }),
			Math::LookAt({ 0.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f }),
			Math::LookAt({ 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }),
			Math::LookAt({ 0.0f, 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }),
			Math::LookAt({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, -1.0f, 0.0f }),
			Math::LookAt({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, -1.0f, 0.0f })
		};
	}

	Skybox::Skybox(UUID equirectangularMapUUID)
		: Skybox()
	{
		m_Enabled = true;
		Initialize(equirectangularMapUUID);
	}

	void Skybox::Initialize(UUID equirectangularMapUUID)
	{
		m_SkyboxTextureHandle = equirectangularMapUUID;

		auto& assetManager = Application::Instance().GetAssetManager();
		auto cubeVao = assetManager.GetAsset<Mesh>(Constants::Assets::CubeMeshUUID)->GetVertexArray();

		RenderAction::UseFaceCulling(false);
		RenderAction::UseDepthTest(false);

		CreateEnvironmentMap(assetManager, cubeVao);
		CreateIrradianceMap(assetManager, cubeVao);
	}

	void Skybox::CreateEnvironmentMap(const AssetManager& assetManager, const SharedPtr<VertexArray>& cubeVAO)
	{
		auto equirectangularShader = assetManager.GetAsset<Shader>(Constants::Assets::EquirectangularToCubemapShad);
		auto equirectangularTexture = assetManager.GetAsset<Texture2D>(m_SkyboxTextureHandle);

		auto skyboxTexture = assetManager.GetAsset<Texture2D>(m_SkyboxTextureHandle);
		m_EnvironmentCubeMap = CubeMap::Create(1024);

		Ember::FramebufferSpecification specs;
		specs.Width = m_Resolution;
		specs.Height = m_Resolution;
		specs.AttachmentSpecs = {
			Ember::FramebufferTextureFormat::RGBA16F,	// Color
			Ember::FramebufferTextureFormat::Depth24	// Depth
		};
		m_SkyboxBuffer = Framebuffer::Create(specs);

		RenderAction::SetViewport(0, 0, m_Resolution, m_Resolution);

		m_SkyboxBuffer->Bind();
		skyboxTexture->Bind(0);

		equirectangularShader->Bind();
		equirectangularShader->SetInt(Constants::Uniforms::EquirectangularMap, 0);	// Bind the equirectangular map as input

		// Render all 6 sides of the cubemap
		equirectangularShader->SetMatrix4(Constants::Uniforms::Projection, m_CaptureProjection);
		for (int i = 0; i < 6; ++i)
		{
			// Attach the current face of the cubemap to the Framebuffer
			m_SkyboxBuffer->AttachColorTextureLayer(m_EnvironmentCubeMap->GetID(), 0, i);
			RenderAction::Clear();

			// Set the camera to look exactly at this face (90 degree FOV)
			equirectangularShader->SetMatrix4(Constants::Uniforms::View, m_CaptureViewMats[i]);

			Renderer3D::Submit(cubeVAO);
		}

		m_SkyboxBuffer->Unbind();
	}

	void Skybox::CreateIrradianceMap(const AssetManager& assetManager, const SharedPtr<VertexArray>& cubeVAO)
	{
		auto irradanceShader = assetManager.GetAsset<Shader>(Constants::Assets::IrradianceShadUUID);

		m_IrradianceMap = CubeMap::Create(32);

		Ember::FramebufferSpecification specs;
		specs.Width = 32;
		specs.Height = 32;
		specs.AttachmentSpecs = {
			Ember::FramebufferTextureFormat::RGBA16F,	// Color
			Ember::FramebufferTextureFormat::Depth24	// Depth
		};
		m_IrradianceBuffer = Framebuffer::Create(specs);

		RenderAction::SetViewport(0, 0, 32, 32);

		m_IrradianceBuffer->Bind();
		m_EnvironmentCubeMap->Bind(0);

		irradanceShader->Bind();
		irradanceShader->SetInt(Constants::Uniforms::EnvironmentMap, 0);	// Bind the environment cubemap as input

		// Render all 6 sides of the cubemap
		irradanceShader->SetMatrix4(Constants::Uniforms::Projection, m_CaptureProjection);
		for (int i = 0; i < 6; ++i)
		{
			// Attach the current face of the cubemap to the Framebuffer
			m_IrradianceBuffer->AttachColorTextureLayer(m_IrradianceMap->GetID(), 0, i);
			RenderAction::Clear();

			irradanceShader->SetMatrix4(Constants::Uniforms::View, m_CaptureViewMats[i]);

			Renderer3D::Submit(cubeVAO);
		}

		m_IrradianceBuffer->Unbind();
	}

}