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

		inline static AssetType GetStaticType() { return AssetType::Mesh; }

		inline uint32_t GetVertexCount() const { return static_cast<uint32_t>(m_VertexArray->GetVertexBuffer()->GetSize() / m_VertexArray->GetVertexBuffer()->GetLayout().GetStride()); }
		inline std::vector<float> GetVertexPositions() const
		{
			std::vector<float> positions;
			const auto& vertexBuffer = m_VertexArray->GetVertexBuffer();
			const auto& layout = vertexBuffer->GetLayout();
			int positionOffset = -1;
			for (const auto& element : layout.GetElements())
			{
				if (element.Name == "v_Position")
				{
					positionOffset = element.Offset;
					break;
				}
			}
			if (positionOffset == -1)
			{
				EB_CORE_ASSERT(false, "Vertex buffer does not contain position attribute!");
				return positions;
			}
			const float* vertexData = reinterpret_cast<const float*>(vertexBuffer->GetData());
			size_t vertexCount = GetVertexCount();
			size_t strideFloats = layout.GetStride() / sizeof(float);
			positions.reserve(vertexCount * 3);
			for (size_t i = 0; i < vertexCount; i++)
			{
				size_t baseIndex = i * strideFloats + positionOffset / sizeof(float);
				positions.push_back(vertexData[baseIndex]);
				positions.push_back(vertexData[baseIndex + 1]);
				positions.push_back(vertexData[baseIndex + 2]);
			}
			return positions;
		}

		inline uint32_t GetTriangleCount() const { return GetIndexCount() / 3; }
		inline std::vector<uint32_t> GetTriangles() const
		{
			std::vector<uint32_t> triangles;
			const auto& indexBuffer = m_VertexArray->GetIndexBuffer();
			const uint32_t* indexData = reinterpret_cast<const uint32_t*>(indexBuffer->GetData());
			size_t indexCount = GetIndexCount();
			triangles.reserve(indexCount);
			for (size_t i = 0; i < indexCount; i++)
			{
				triangles.push_back(indexData[i]);
			}
			return triangles;
		}

	protected:
		SharedPtr<VertexArray> m_VertexArray;
	};
}