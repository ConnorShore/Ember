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
		auto equirectangularShader = assetManager.GetAsset<Shader>(Constants::Assets::EquirectangularToCubemapShad);
		auto equirectangularTexture = assetManager.GetAsset<Texture2D>(equirectangularMapUUID);
		auto cubeVao = assetManager.GetAsset<Mesh>(Constants::Assets::CubeMeshUUID)->GetVertexArray();

		auto skyboxTexture = assetManager.GetAsset<Texture2D>(equirectangularMapUUID);
		m_EnvironmentCubeMap = CubeMap::Create(1024);

		Ember::FramebufferSpecification skyboxFBOSpecs;
		skyboxFBOSpecs.Width = m_Resolution;
		skyboxFBOSpecs.Height = m_Resolution;
		skyboxFBOSpecs.AttachmentSpecs = {
			Ember::FramebufferTextureFormat::RGBA16F,	// Color
			Ember::FramebufferTextureFormat::Depth24	// Depth
		};
		m_SkyboxBuffer = Framebuffer::Create(skyboxFBOSpecs);

		RenderAction::SetViewport(0, 0, m_Resolution, m_Resolution);

		RenderAction::UseFaceCulling(false);
		RenderAction::UseDepthTest(false);

		m_SkyboxBuffer->Bind();
		skyboxTexture->Bind(0);
		equirectangularShader->Bind();
		equirectangularShader->SetInt(Constants::Uniforms::EquirectangularMap, 0);

		// Render all 6 sides of the cubemap
		equirectangularShader->SetMatrix4(Constants::Uniforms::Projection, m_CaptureProjection);
		for (int i = 0; i < 6; ++i)
		{
			// Attach the current face of the cubemap to the Framebuffer
			m_SkyboxBuffer->AttachColorTextureLayer(m_EnvironmentCubeMap->GetID(), 0, i);
			RenderAction::Clear();

			// Set the camera to look exactly at this face (90 degree FOV)
			equirectangularShader->SetMatrix4(Constants::Uniforms::View, m_CaptureViewMats[i]);

			Renderer3D::Submit(cubeVao);
		}

		m_SkyboxBuffer->Unbind();
	}

}