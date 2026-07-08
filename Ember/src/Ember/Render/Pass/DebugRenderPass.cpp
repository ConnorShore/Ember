#include "ebpch.h"
#include "DebugRenderPass.h"

#include "Ember/Render/DebugRenderer.h"
#include "Ember/Render/RenderAction.h"

namespace Ember {

	
	void DebugRenderPass::Init()
	{
		m_DebugLineVBO = VertexBuffer::Create(MaxDebugVertices * sizeof(DebugVertex));
		m_DebugLineVBO->SetLayout({
			{ ShaderDataType::Float3, "v_Position" },
			{ ShaderDataType::Float4, "v_Color" }
			});

		m_DebugLineVAO = VertexArray::Create();
		m_DebugLineVAO->AddVertexBuffer(m_DebugLineVBO);

		m_FilledTriangleVBO = VertexBuffer::Create(MaxDebugVertices * sizeof(DebugVertex));
		m_FilledTriangleVBO->SetLayout({
			{ ShaderDataType::Float3, "v_Position" },
			{ ShaderDataType::Float4, "v_Color" }
			});

		m_FilledTriangleVAO = VertexArray::Create();
		m_FilledTriangleVAO->AddVertexBuffer(m_FilledTriangleVBO);
	}

	void DebugRenderPass::Execute(RenderContext& context)
	{
		const auto& triangleVertices = DebugRenderer::GetFilledTriangleVertices();
		const auto& vertices = DebugRenderer::GetVertices();

		if (!triangleVertices.empty())
		{
			auto physicsDebugShader = Application::Instance().GetAssetManager().GetAsset<Shader>(Constants::Assets::PhysicsDebugShadUUID);
			physicsDebugShader->Bind();

			RenderAction::UseDepthTest(true);
			RenderAction::UseDepthMask(false);
			RenderAction::UseBlending(true);

			uint32_t requiredSize = static_cast<uint32_t>(triangleVertices.size() * sizeof(DebugVertex));
			if (requiredSize > m_FilledTriangleVBO->GetSize())
			{
				m_FilledTriangleVBO = VertexBuffer::Create(requiredSize);
				m_FilledTriangleVBO->SetLayout({
					{ ShaderDataType::Float3, "v_Position" },
					{ ShaderDataType::Float4, "v_Color" }
					});
				m_FilledTriangleVAO = VertexArray::Create();
				m_FilledTriangleVAO->AddVertexBuffer(m_FilledTriangleVBO);
			}

			m_FilledTriangleVBO->SetData(triangleVertices.data(), requiredSize);
			m_FilledTriangleVAO->Bind();
			RenderAction::DrawTriangles(m_FilledTriangleVAO, static_cast<uint32_t>(triangleVertices.size()));

			RenderAction::UseBlending(false);
			RenderAction::UseDepthMask(true);
		}

		if (!vertices.empty())
		{
			auto physicsDebugShader = Application::Instance().GetAssetManager().GetAsset<Shader>(Constants::Assets::PhysicsDebugShadUUID);
			physicsDebugShader->Bind();

			uint32_t requiredSize = static_cast<uint32_t>(vertices.size() * sizeof(DebugVertex));
			if (requiredSize > m_DebugLineVBO->GetSize())
			{
				// Recreate both the VBO and VAO — AddVertexBuffer uses push_back slot indexing,
					// so reusing the old VAO would bind the new VBO to slot 1 while attributes
					// still reference slot 0, causing partial/no rendering.
					m_DebugLineVBO = VertexBuffer::Create(requiredSize);
					m_DebugLineVBO->SetLayout({
						{ ShaderDataType::Float3, "v_Position" },
						{ ShaderDataType::Float4, "v_Color" }
						});
					m_DebugLineVAO = VertexArray::Create();
					m_DebugLineVAO->AddVertexBuffer(m_DebugLineVBO);
			}

			m_DebugLineVBO->SetData(vertices.data(), requiredSize);
			m_DebugLineVAO->Bind();
			RenderAction::DrawLines(m_DebugLineVAO, static_cast<uint32_t>(vertices.size()));
		}

		DebugRenderer::Clear();
	}

	void DebugRenderPass::OnViewportResize(uint32_t width, uint32_t height)
	{

	}

	void DebugRenderPass::Shutdown()
	{
	}

}