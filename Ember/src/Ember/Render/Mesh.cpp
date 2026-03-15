#include "ebpch.h"
#include "Mesh.h"

namespace Ember {

	//////////////////////////////////////////////////////////////////////////
	// Mesh
	//////////////////////////////////////////////////////////////////////////

	Mesh::Mesh(const std::string& name, const std::string& filePath)
		: Asset(name, filePath, AssetType::Mesh)
	{
		// TODO: Model loading
		// 
		//auto vbo = VertexBuffer::Create(vertices, sizeof(vertices), {
		//	{ ShaderDataType::Float3, "v_Position" },
		//	{ ShaderDataType::Float3, "v_Normal" },
		//	{ ShaderDataType::Float2, "v_TextureCoord"}
		//	});
		//auto ibo = IndexBuffer::Create(indices);

		//m_VertexArray = VertexArray::Create();
		//m_VertexArray->SetBuffer(vbo, ibo);
	}

	Mesh::Mesh(const std::string& filePath)
		: Mesh(std::filesystem::path(filePath).stem().string(), filePath)
	{
	}

	Mesh::Mesh(const std::string& name, std::vector<float>& vertices, const std::vector<unsigned int>& indices)
		: Asset(name, "", AssetType::Mesh)
	{
		auto vbo = VertexBuffer::Create(&vertices[0], sizeof(float) * vertices.size(), {
			{ ShaderDataType::Float3, "v_Position" },
			{ ShaderDataType::Float3, "v_Normal" },
			{ ShaderDataType::Float2, "v_TextureCoord"}
		});
		auto ibo = IndexBuffer::Create(indices);

		m_VertexArray = VertexArray::Create();
		m_VertexArray->SetBuffer(vbo, ibo);
	}

	Mesh::~Mesh()
	{

	}

}