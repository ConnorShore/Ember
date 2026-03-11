#include "ebpch.h"
#include "Mesh.h"

namespace Ember {

	// Dummy test data

	float vertices[] = {
		// Position (X, Y, Z)       // Color (R, G, B, A)
		-0.5f, -0.5f,  0.5f,        1.0f, 0.0f, 0.0f, 1.0f, // 0: Bottom-Left-Front (Red)
		 0.5f, -0.5f,  0.5f,        0.0f, 1.0f, 0.0f, 1.0f, // 1: Bottom-Right-Front (Green)
		 0.5f,  0.5f,  0.5f,        0.0f, 0.0f, 1.0f, 1.0f, // 2: Top-Right-Front (Blue)
		-0.5f,  0.5f,  0.5f,        1.0f, 1.0f, 0.0f, 1.0f, // 3: Top-Left-Front (Yellow)
		-0.5f, -0.5f, -0.5f,        1.0f, 0.0f, 1.0f, 1.0f, // 4: Bottom-Left-Back (Magenta)
		 0.5f, -0.5f, -0.5f,        0.0f, 1.0f, 1.0f, 1.0f, // 5: Bottom-Right-Back (Cyan)
		 0.5f,  0.5f, -0.5f,        1.0f, 1.0f, 1.0f, 1.0f, // 6: Top-Right-Back (White)
		-0.5f,  0.5f, -0.5f,        0.0f, 0.0f, 0.0f, 1.0f  // 7: Top-Left-Back (Black)
	};

	uint32_t indices[] = {
		0, 1, 2, 2, 3, 0, // Front face
		1, 5, 6, 6, 2, 1, // Right face
		5, 4, 7, 7, 6, 5, // Back face
		4, 0, 3, 3, 7, 4, // Left face
		3, 2, 6, 6, 7, 3, // Top face
		4, 5, 1, 1, 0, 4  // Bottom face
	};

	//////////////////////////////////////////////////////////////////////////
	// Mesh
	//////////////////////////////////////////////////////////////////////////

	Mesh::Mesh(const std::string& name, const std::string& filePath)
		: m_Name(name)
	{
		auto vbo = VertexBuffer::Create(vertices, sizeof(vertices), {
			{ ShaderDataType::Float3, "v_Position" },
			{ ShaderDataType::Float4, "v_Color" }
			});
		auto ibo = IndexBuffer::Create(indices);

		m_VertexArray = VertexArray::Create();
		m_VertexArray->SetBuffer(vbo, ibo);
	}

	Mesh::Mesh(const std::string& filePath)
		: Mesh(std::filesystem::path(filePath).stem().string(), filePath)
	{
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