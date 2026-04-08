#pragma once

#include "Ember/Core/Core.h"
#include "Ember/Render/VertexArray.h"
#include "Ember/Render/Buffer.h"
#include "Ember/Asset/Asset.h"
#include "Ember/Asset/MeshHeader.h"

#include <unordered_map>

namespace Ember {

	class Mesh : public Asset
	{
	public:
		Mesh(UUID uuid, const std::string& name) 
			: Asset(uuid, name, "", GetStaticType()) {}
		virtual ~Mesh() = default;

		inline const SharedPtr<VertexArray>& GetVertexArray() { return m_VertexArray; }
		inline uint32_t GetIndexCount() const { return static_cast<uint32_t>(m_VertexArray->GetIndexBuffer()->GetCount()); }

		static AssetType GetStaticType() { return AssetType::Mesh; }

	protected:
		SharedPtr<VertexArray> m_VertexArray;
	};
}