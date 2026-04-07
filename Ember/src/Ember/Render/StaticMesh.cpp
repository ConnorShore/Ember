#include "ebpch.h"
#include "StaticMesh.h"

namespace Ember {

	StaticMesh::StaticMesh(UUID uuid, const std::string& name, const std::vector<float>& vertices, const std::vector<uint32_t>& indices)
		: Mesh(uuid, name)
	{
		auto vbo = VertexBuffer::Create(&vertices[0], static_cast<uint32_t>(sizeof(float) * vertices.size()), {
			{ ShaderDataType::Float3, "v_Position" },
			{ ShaderDataType::Float3, "v_Normal" },
			{ ShaderDataType::Float2, "v_TextureCoord"},
			{ ShaderDataType::Float3, "v_Tangent" },
			{ ShaderDataType::Float3, "v_Bitangent" }
			});
		auto ibo = IndexBuffer::Create(indices);

		m_VertexArray = VertexArray::Create();
		m_VertexArray->SetBuffer(vbo, ibo);
	}

	StaticMesh::StaticMesh(const std::string& name, const std::vector<float>& vertices, const std::vector<uint32_t>& indices)
		: StaticMesh(UUID(), name, vertices, indices)
	{
	}

	StaticMesh::StaticMesh(UUID uuid, const std::string& name, const std::vector<StaticMeshVertex>& vertices, const std::vector<uint32_t>& indices)
		: Mesh(uuid, name)
	{
		auto vbo = VertexBuffer::Create(&vertices[0], static_cast<uint32_t>(sizeof(StaticMeshVertex) * vertices.size()), {
			{ ShaderDataType::Float3, "v_Position" },
			{ ShaderDataType::Float3, "v_Normal" },
			{ ShaderDataType::Float2, "v_TextureCoord"},
			{ ShaderDataType::Float3, "v_Tangent" },
			{ ShaderDataType::Float3, "v_Bitangent" }
			});
		auto ibo = IndexBuffer::Create(indices);

		m_VertexArray = VertexArray::Create();
		m_VertexArray->SetBuffer(vbo, ibo);
	}

	StaticMesh::~StaticMesh()
	{

	}
}