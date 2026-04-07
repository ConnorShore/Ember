#include "ebpch.h"
#include "Skybox.h"
#include "RenderAction.h"
#include "Renderer3D.h"
#include "PrimitiveGenerator.h"

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
		CreatePrefilteredMap(assetManager, cubeVao);
		CreateBRDFLUT(assetManager);
	}

	void Skybox::CreateEnvironmentMap(const AssetManager& assetManager, const SharedPtr<VertexArray>& cubeVAO)
	{
		auto equirectangularShader = assetManager.GetAsset<Shader>(Constants::Assets::EquirectangularToCubemapShad);
		auto equirectangularTexture = assetManager.GetAsset<Texture2D>(m_SkyboxTextureHandle);

		auto skyboxTexture = assetManager.GetAsset<Texture2D>(m_SkyboxTextureHandle);
		m_EnvironmentCubeMap = CubeMap::Create(m_Resolution);

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
		for (int i = 0; i < 6; i++)
		{
			// Set the camera to look exactly at this face (90 degree FOV)
			equirectangularShader->SetMatrix4(Constants::Uniforms::View, m_CaptureViewMats[i]);

			// Attach the current face of the cubemap to the Framebuffer
			m_SkyboxBuffer->AttachColorTextureLayer(m_EnvironmentCubeMap->GetID(), 0, i);

			RenderAction::Clear(Ember::RendererAPI::RenderBit::Color | Ember::RendererAPI::RenderBit::Depth);
			Renderer3D::Submit(cubeVAO);
		}

		m_EnvironmentCubeMap->GenerateMipmaps();
		m_SkyboxBuffer->Unbind();
	}

	void Skybox::CreateIrradianceMap(const AssetManager& assetManager, const SharedPtr<VertexArray>& cubeVAO)
	{
		auto irradianceShader = assetManager.GetAsset<Shader>(Constants::Assets::IrradianceShadUUID);
		auto irradianceResolution = 32;

		m_IrradianceMap = CubeMap::Create(32);

		Ember::FramebufferSpecification specs;
		specs.Width = irradianceResolution;
		specs.Height = irradianceResolution;
		specs.AttachmentSpecs = {
			Ember::FramebufferTextureFormat::RGBA16F,	// Color
			Ember::FramebufferTextureFormat::Depth24	// Depth
		};
		m_IrradianceBuffer = Framebuffer::Create(specs);

		RenderAction::SetViewport(0, 0, irradianceResolution, irradianceResolution);

		m_IrradianceBuffer->Bind();
		m_EnvironmentCubeMap->Bind(0);

		irradianceShader->Bind();
		irradianceShader->SetInt(Constants::Uniforms::EnvironmentMap, 0);	// Bind the environment cubemap as input

		// Render all 6 sides of the cubemap
		irradianceShader->SetMatrix4(Constants::Uniforms::Projection, m_CaptureProjection);
		for (int i = 0; i < 6; i++)
		{
			irradianceShader->SetMatrix4(Constants::Uniforms::View, m_CaptureViewMats[i]);

			// Attach the current face of the cubemap to the Framebuffer
			m_IrradianceBuffer->AttachColorTextureLayer(m_IrradianceMap->GetID(), 0, i);

			RenderAction::Clear(Ember::RendererAPI::RenderBit::Color | Ember::RendererAPI::RenderBit::Depth);
			Renderer3D::Submit(cubeVAO);
		}

		m_IrradianceMap->GenerateMipmaps();
		m_IrradianceBuffer->Unbind();
	}

	void Skybox::CreatePrefilteredMap(const AssetManager& assetManager, const SharedPtr<VertexArray>& cubeVAO)
	{
		auto prefilterShader = assetManager.GetAsset<Shader>(Constants::Assets::PrefilterShadUUID);
		uint32_t prefilterResolution = 128;

		m_PrefilteredMap = CubeMap::Create(prefilterResolution);

		Ember::FramebufferSpecification specs;
		specs.Width = prefilterResolution;
		specs.Height = prefilterResolution;
		specs.AttachmentSpecs = {
			Ember::FramebufferTextureFormat::RGBA16F,	// Color
			Ember::FramebufferTextureFormat::Depth24	// Depth
		};
		m_PrefilterBuffer = Framebuffer::Create(specs);

		RenderAction::SetViewport(0, 0, prefilterResolution, prefilterResolution);

		// Generate mipmaps for the environment cubemap so we can sample different roughness levels in the shader
		m_EnvironmentCubeMap->GenerateMipmaps();

		m_PrefilterBuffer->Bind();
		m_EnvironmentCubeMap->Bind(0);

		prefilterShader->Bind();
		prefilterShader->SetInt(Constants::Uniforms::EnvironmentMap, 0);	// Bind the environment cubemap as input

		prefilterShader->SetInt("u_NumSamples", 1024);
		prefilterShader->SetFloat("u_Resolution", static_cast<float>(m_Resolution));

		uint32_t maxMipLevels = m_PrefilteredMap->GetNumMipMapLevels();
		for (int mip = 0; mip < maxMipLevels; mip++)
		{
			// Resize the viewport for each mip level
			unsigned int mipWidth = static_cast<unsigned int>(prefilterResolution * std::pow(0.5, mip));
			unsigned int mipHeight = static_cast<unsigned int>(prefilterResolution * std::pow(0.5, mip));
			RenderAction::SetViewport(0, 0, mipWidth, mipHeight);

			float roughness = (float)mip / (float)(maxMipLevels - 1);
			prefilterShader->SetFloat(Constants::Uniforms::Roughness, roughness);

			// Render all 6 sides of the cubemap
			prefilterShader->SetMatrix4(Constants::Uniforms::Projection, m_CaptureProjection);
			for (size_t i = 0; i < 6; i++)
			{
				prefilterShader->SetMatrix4(Constants::Uniforms::View, m_CaptureViewMats[i]);

				// Attach the current face of the cubemap to the Framebuffer
				m_PrefilterBuffer->AttachColorTextureLayer(m_PrefilteredMap->GetID(), mip, i);
				RenderAction::Clear(Ember::RendererAPI::RenderBit::Color | Ember::RendererAPI::RenderBit::Depth);
				Renderer3D::Submit(cubeVAO);
			}
		}

		m_PrefilterBuffer->Unbind();
	}

	void Skybox::CreateBRDFLUT(const AssetManager& assetManager)
	{
		// Must use a 2x2 quad so it covers full NDC range (-1 to 1), not the default 1x1 asset quad
		auto fullscreenQuad = PrimitiveGenerator::CreateQuad(2.0f, 2.0f);
		auto quadVao = fullscreenQuad->GetVertexArray();

		m_BRDFLUT = Texture2D::Create(UUID(), "BRDFlut", TextureFormat::RG16F, m_Resolution, m_Resolution, nullptr, true);

		Ember::FramebufferSpecification specs;
		specs.Width = m_Resolution;
		specs.Height = m_Resolution;
		specs.AttachmentSpecs = {
			Ember::FramebufferTextureFormat::RGBA16F,	// Color
			Ember::FramebufferTextureFormat::Depth24	// Depth
		};
		m_BRDFLUTBuffer = Framebuffer::Create(specs);
		RenderAction::SetViewport(0, 0, m_Resolution, m_Resolution);

		m_BRDFLUTBuffer->Bind();
		m_BRDFLUTBuffer->AttachColorTexture(m_BRDFLUT->GetID(), 0);

		auto brdfShader = assetManager.GetAsset<Shader>(Constants::Assets::BRDFLUTShadUUID);
		brdfShader->Bind();
		brdfShader->SetInt("u_NumSamples", 1024);

		RenderAction::Clear(Ember::RendererAPI::RenderBit::Color | Ember::RendererAPI::RenderBit::Depth);
		Renderer3D::Submit(quadVao);

		m_BRDFLUTBuffer->Unbind();
	}

}