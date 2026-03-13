#pragma once

#include "Ember/Core/Core.h"
#include "Ember/Render/VertexArray.h"
#include "Ember/Render/Buffer.h"

#include <unordered_map>

namespace Ember {

	//////////////////////////////////////////////////////////////////////////
	// Mesh
	//////////////////////////////////////////////////////////////////////////

	class Mesh : public SharedResource
	{
	public:
		Mesh(const std::string& filePath);
		Mesh(const std::string& name, const std::string& filePath);
		Mesh(const std::vector<float>& vertices, const std::vector<unsigned int>& indices);
		~Mesh();

		inline const SharedPtr<VertexArray>& GetVertexArray() { return m_VertexArray; }
		inline unsigned int GetIndexCount() const { return m_VertexArray->GetIndexBuffer()->GetCount(); }
		inline const std::string& GetName() { return m_Name; }

	private:
		SharedPtr<VertexArray> m_VertexArray;
		std::string m_Name;
	};

	//////////////////////////////////////////////////////////////////////////
	// Mesh Library
	//////////////////////////////////////////////////////////////////////////

	class MeshLibrary
	{
	public:
		const SharedPtr<Mesh>& Register(const std::string& filePath);
		const SharedPtr<Mesh>& Register(const std::string& name, const std::string& filePath);

		const SharedPtr<Mesh>& Get(const std::string& name);
		bool Exists(const std::string& name);

	private:
		void Add(SharedPtr<Mesh>&& mesh);
		void Add(const std::string& name, SharedPtr<Mesh>&& mesh);
	private:
		std::unordered_map<std::string, SharedPtr<Mesh>> m_MeshMap;
	};
}