#pragma once

#include "Asset.h"
#include "Ember/Core/Core.h"
#include "Ember/Math/Math.h"
#include "Ember/Render/Mesh.h"
#include "Ember/Render/Material.h"

#include <vector>
#include <string>

namespace Ember {

	struct MeshMaterialNode
	{
		SharedPtr<Mesh> MeshAsset;
		unsigned int MaterialIndex;
	};

	struct ModelNode
	{
		std::string Name;
		Matrix4f LocalTransform = Matrix4f(1.0f);
		std::vector<ModelNode> ChildNodes;
		std::vector<MeshMaterialNode> Meshes;

		ModelNode() = default;
		ModelNode(const ModelNode& other) = default;
	};

	class Model : public Asset
	{
	public:
		Model(UUID uuid, const std::string& name, const std::string& filePath, const ModelNode& rootNode, const std::vector<SharedPtr<MaterialBase>>& materials)
			: Asset(uuid, name, filePath, GetStaticType()), m_RootNode(rootNode), m_AllMaterials(materials)
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
		Model(const std::string& name, const std::string& filePath, const ModelNode& rootNode, const std::vector<SharedPtr<MaterialBase>>& materials)
			: Model(UUID(), name, filePath, rootNode, materials)
		{
		}

		~Model() = default;

		const ModelNode& GetRootNode() const { return m_RootNode; }
		const std::vector<MeshMaterialNode>& GetAllMeshes() const { return m_AllMeshes; }
		const std::vector<SharedPtr<MaterialBase>>& GetAllMaterials() const { return m_AllMaterials; }

		static AssetType GetStaticType() { return AssetType::Model; }

	private:
		ModelNode m_RootNode;
		std::vector<MeshMaterialNode> m_AllMeshes;
		std::vector<SharedPtr<MaterialBase>> m_AllMaterials;
	};

}