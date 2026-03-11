#include "ebpch.h"
#include "Mesh.h"

namespace Ember {

	// Dummy test data

	float vertices[] = {
		// Front Face (Normal: 0, 0, 1)
		-0.5f, -0.5f,  0.5f,    0.0f,  0.0f,  1.0f,    0.0f, 0.0f, // 0: Bottom-Left
		 0.5f, -0.5f,  0.5f,    0.0f,  0.0f,  1.0f,    1.0f, 0.0f, // 1: Bottom-Right
		 0.5f,  0.5f,  0.5f,    0.0f,  0.0f,  1.0f,    1.0f, 1.0f, // 2: Top-Right
		-0.5f,  0.5f,  0.5f,    0.0f,  0.0f,  1.0f,    0.0f, 1.0f, // 3: Top-Left

		// Right Face (Normal: 1, 0, 0)
		 0.5f, -0.5f,  0.5f,    1.0f,  0.0f,  0.0f,    0.0f, 0.0f, // 4: Bottom-Left
		 0.5f, -0.5f, -0.5f,    1.0f,  0.0f,  0.0f,    1.0f, 0.0f, // 5: Bottom-Right
		 0.5f,  0.5f, -0.5f,    1.0f,  0.0f,  0.0f,    1.0f, 1.0f, // 6: Top-Right
		 0.5f,  0.5f,  0.5f,    1.0f,  0.0f,  0.0f,    0.0f, 1.0f, // 7: Top-Left

		 // Back Face (Normal: 0, 0, -1)
		  0.5f, -0.5f, -0.5f,    0.0f,  0.0f, -1.0f,    0.0f, 0.0f, // 8: Bottom-Left
		 -0.5f, -0.5f, -0.5f,    0.0f,  0.0f, -1.0f,    1.0f, 0.0f, // 9: Bottom-Right
		 -0.5f,  0.5f, -0.5f,    0.0f,  0.0f, -1.0f,    1.0f, 1.0f, // 10: Top-Right
		  0.5f,  0.5f, -0.5f,    0.0f,  0.0f, -1.0f,    0.0f, 1.0f, // 11: Top-Left

		  // Left Face (Normal: -1, 0, 0)
		  -0.5f, -0.5f, -0.5f,   -1.0f,  0.0f,  0.0f,    0.0f, 0.0f, // 12: Bottom-Left
		  -0.5f, -0.5f,  0.5f,   -1.0f,  0.0f,  0.0f,    1.0f, 0.0f, // 13: Bottom-Right
		  -0.5f,  0.5f,  0.5f,   -1.0f,  0.0f,  0.0f,    1.0f, 1.0f, // 14: Top-Right
		  -0.5f,  0.5f, -0.5f,   -1.0f,  0.0f,  0.0f,    0.0f, 1.0f, // 15: Top-Left

		  // Top Face (Normal: 0, 1, 0)
		  -0.5f,  0.5f,  0.5f,    0.0f,  1.0f,  0.0f,    0.0f, 0.0f, // 16: Bottom-Left
		   0.5f,  0.5f,  0.5f,    0.0f,  1.0f,  0.0f,    1.0f, 0.0f, // 17: Bottom-Right
		   0.5f,  0.5f, -0.5f,    0.0f,  1.0f,  0.0f,    1.0f, 1.0f, // 18: Top-Right
		  -0.5f,  0.5f, -0.5f,    0.0f,  1.0f,  0.0f,    0.0f, 1.0f, // 19: Top-Left

		  // Bottom Face (Normal: 0, -1, 0)
		  -0.5f, -0.5f, -0.5f,    0.0f, -1.0f,  0.0f,    0.0f, 0.0f, // 20: Bottom-Left
		   0.5f, -0.5f, -0.5f,    0.0f, -1.0f,  0.0f,    1.0f, 0.0f, // 21: Bottom-Right
		   0.5f, -0.5f,  0.5f,    0.0f, -1.0f,  0.0f,    1.0f, 1.0f, // 22: Top-Right
		  -0.5f, -0.5f,  0.5f,    0.0f, -1.0f,  0.0f,    0.0f, 1.0f  // 23: Top-Left
	};

	uint32_t indices[] = {
		 0,  1,  2,  2,  3,  0, // Front
		 4,  5,  6,  6,  7,  4, // Right
		 8,  9, 10, 10, 11,  8, // Back
		12, 13, 14, 14, 15, 12, // Left
		16, 17, 18, 18, 19, 16, // Top
		20, 21, 22, 22, 23, 20  // Bottom
	};

	//////////////////////////////////////////////////////////////////////////
	// Mesh
	//////////////////////////////////////////////////////////////////////////

	Mesh::Mesh(const std::string& name, const std::string& filePath)
		: m_Name(name)
	{
		auto vbo = VertexBuffer::Create(vertices, sizeof(vertices), {
			{ ShaderDataType::Float3, "v_Position" },
			{ ShaderDataType::Float3, "v_Normal" },
			{ ShaderDataType::Float2, "v_TextureCoord"}
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