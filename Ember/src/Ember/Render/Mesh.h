#pragma once

#include "Ember/Core/Core.h"
#include "Ember/Render/VertexArray.h"
#include "Ember/Render/Buffer.h"
#include "Ember/Asset/Asset.h"

#include <unordered_map>

namespace Ember {

	//////////////////////////////////////////////////////////////////////////
	// Mesh
	//////////////////////////////////////////////////////////////////////////

	class Mesh : public Asset
	{
	public:
		Mesh(const std::string& name, const std::vector<float>& vertices, const std::vector<unsigned int>& indices);
		Mesh(UUID uuid, const std::string& name, const std::vector<float>& vertices, const std::vector<unsigned int>& indices);
		~Mesh();

		inline const SharedPtr<VertexArray>& GetVertexArray() { return m_VertexArray; }
		inline unsigned int GetIndexCount() const { return m_VertexArray->GetIndexBuffer()->GetCount(); }

		static AssetType GetStaticType() { return AssetType::Mesh; }

	private:
		SharedPtr<VertexArray> m_VertexArray;
	};
}