#include "ebpch.h"
#include "SkinnedMesh.h"

namespace Ember {
	SkinnedMesh::SkinnedMesh(UUID uuid, const std::string& name, const std::vector<float>& vertices, const std::vector<uint32_t>& indices)
		: Mesh(uuid, name)
	{
		auto vbo = VertexBuffer::Create(&vertices[0], static_cast<uint32_t>(sizeof(float) * vertices.size()), {
			{ ShaderDataType::Float3, "v_Position" },
			{ ShaderDataType::Float3, "v_Normal" },
			{ ShaderDataType::Float2, "v_TextureCoord"},
			{ ShaderDataType::Float3, "v_Tangent" },
			{ ShaderDataType::Float3, "v_Bitangent" },
			{ ShaderDataType::UInt4,  "v_BoneIDs" },
			{ ShaderDataType::Float4, "v_Weights" }
			});
		auto ibo = IndexBuffer::Create(indices);

		m_VertexArray = VertexArray::Create();
		m_VertexArray->SetBuffer(vbo, ibo);
	}

	SkinnedMesh::SkinnedMesh(const std::string& name, const std::vector<float>& vertices, const std::vector<uint32_t>& indices)
		: SkinnedMesh(UUID(), name, vertices, indices)
	{
	}

	SkinnedMesh::SkinnedMesh(UUID uuid, const std::string& name, const std::vector<SkinnedMeshVertex>& vertices, const std::vector<uint32_t>& indices)
		: Mesh(uuid, name)
	{
		auto vbo = VertexBuffer::Create(&vertices[0], static_cast<uint32_t>(sizeof(SkinnedMeshVertex) * vertices.size()), {
			{ ShaderDataType::Float3, "v_Position" },
			{ ShaderDataType::Float3, "v_Normal" },
			{ ShaderDataType::Float2, "v_TextureCoord"},
			{ ShaderDataType::Float3, "v_Tangent" },
			{ ShaderDataType::Float3, "v_Bitangent" },
			{ ShaderDataType::UInt4,  "v_BoneIDs" },
			{ ShaderDataType::Float4, "v_Weights" }
			});
		auto ibo = IndexBuffer::Create(indices);

		m_VertexArray = VertexArray::Create();
		m_VertexArray->SetBuffer(vbo, ibo);
	}

	SkinnedMesh::~SkinnedMesh()
	{

	}
}