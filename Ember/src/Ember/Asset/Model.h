#pragma once

#include "Asset.h"
#include "Ember/Core/Core.h"
#include "Ember/Math/Math.h"
#include "Ember/Render/Mesh.h"
#include "Ember/Render/Material.h"

#include <vector>
#include <string>

namespace Ember {

	// TODO: Integrate materials into the model structure
	struct MeshMaterialNode
	{
		SharedPtr<Mesh> MeshAsset;
		uint32_t MaterialIndex;
	};

	struct ModelNode
	{
		std::string Name;
		Matrix4f LocalTransform = Matrix4f(1.0f);
		std::vector<ModelNode> ChildNodes;
		std::vector<SharedPtr<Mesh>> Meshes;

		ModelNode() = default;
		ModelNode(const ModelNode& other) = default;
	};

	class Model : public Asset
	{
	public:
		Model(const std::string& name, const std::string& filePath, const ModelNode& rootNode)
			: Asset(name, filePath, AssetType::Model), m_RootNode(rootNode)
		{
			std::vector<const ModelNode*> nodesToVisit = { &m_RootNode };
			while (!nodesToVisit.empty())
			{
				const ModelNode* currentNode = nodesToVisit.back();
				nodesToVisit.pop_back();

				for (const auto& mesh : currentNode->Meshes)
					m_AllMeshes.push_back(mesh);

				for (const auto& childNode : currentNode->ChildNodes)
					nodesToVisit.push_back(&childNode);
			}
		}

		~Model() = default;

		const ModelNode& GetRootNode() const { return m_RootNode; }
		const std::vector<SharedPtr<Mesh>>& GetAllMeshes() const { return m_AllMeshes; }
		const std::vector<SharedPtr<Material>>& GetAllMaterials() const { return m_AllMaterials; }

	private:
		ModelNode m_RootNode;
		std::vector<SharedPtr<Mesh>> m_AllMeshes;
		std::vector<SharedPtr<Material>> m_AllMaterials;
	};

}