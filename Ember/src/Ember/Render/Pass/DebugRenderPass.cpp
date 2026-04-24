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
	}

	void DebugRenderPass::Execute(RenderContext& context)
	{
		const auto& vertices = DebugRenderer::GetVertices();

		if (!vertices.empty())
		{
			auto physicsDebugShader = Application::Instance().GetAssetManager().GetAsset<Shader>(Constants::Assets::PhysicsDebugShadUUID);
			physicsDebugShader->Bind();

			uint32_t requiredSize = static_cast<uint32_t>(vertices.size() * sizeof(DebugVertex));
			if (requiredSize > m_DebugLineVBO->GetSize())
			{
				// Immutable GPU storage cannot be resized — recreate the VBO and rebind it to the VAO
				m_DebugLineVBO = VertexBuffer::Create(requiredSize);
				m_DebugLineVBO->SetLayout({
					{ ShaderDataType::Float3, "v_Position" },
					{ ShaderDataType::Float4, "v_Color" }
					});
				m_DebugLineVAO->AddVertexBuffer(m_DebugLineVBO);
			}

			m_DebugLineVBO->SetData(vertices.data(), requiredSize);
			m_DebugLineVAO->Bind();
			RenderAction::DrawLines(m_DebugLineVAO, static_cast<uint32_t>(vertices.size()));
		}

		DebugRenderer::Clear();
	}

	void DebugRenderPass::Shutdown()
	{
	}

}