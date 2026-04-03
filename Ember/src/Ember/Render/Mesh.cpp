#include "ebpch.h"
#include "Mesh.h"

namespace Ember {

	//////////////////////////////////////////////////////////////////////////
	// Mesh
	//////////////////////////////////////////////////////////////////////////

	Mesh::Mesh(UUID uuid, const std::string& name, const std::vector<float>& vertices, const std::vector<uint32_t>& indices)
		: Asset(uuid, name, "", GetStaticType())
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

	Mesh::Mesh(const std::string& name, const std::vector<float>& vertices, const std::vector<uint32_t>& indices)
		: Mesh(UUID(), name, vertices, indices)
	{
	}

	Mesh::Mesh(UUID uuid, const std::string& name, const std::vector<MeshVertex>& vertices, const std::vector<uint32_t>& indices)
		: Asset(uuid, name, "", GetStaticType())
	{
		auto vbo = VertexBuffer::Create(&vertices[0], static_cast<uint32_t>(sizeof(MeshVertex) * vertices.size()), {
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

	Mesh::~Mesh()
	{

	}

}