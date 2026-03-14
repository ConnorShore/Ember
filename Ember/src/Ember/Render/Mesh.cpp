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

	//////////////////////////////////////////////////////////////////////////
	// Mesh Library
	//////////////////////////////////////////////////////////////////////////

	const SharedPtr<Mesh>& MeshLibrary::Register(const std::string& filePath)
	{
		auto mesh = SharedPtr<Mesh>::Create(filePath);
		Add(std::move(mesh));
		return Get(std::filesystem::path(filePath).stem().string());
	}

	const SharedPtr<Mesh>& MeshLibrary::Register(const std::string& name, const std::string& filePath)
	{
		auto mesh = SharedPtr<Mesh>::Create(name, filePath);
		Add(std::move(mesh));
		return Get(name);
	}

	const SharedPtr<Mesh>& MeshLibrary::Get(const std::string& name)
	{
		EB_CORE_ASSERT(Exists(name), "Mesh does not exists in library!");
		return m_MeshMap.at(name);
	}

	bool MeshLibrary::Exists(const std::string& name)
	{
		return m_MeshMap.contains(name);
	}

	void MeshLibrary::Add(SharedPtr<Mesh>&& mesh)
	{
		Add(mesh->GetName(), std::move(mesh));
	}

	void MeshLibrary::Add(const std::string& name, SharedPtr<Mesh>&& mesh)
	{
		EB_CORE_ASSERT(!Exists(name), "Mesh already exists in library!");
		m_MeshMap[name] = std::move(mesh);
	}

}