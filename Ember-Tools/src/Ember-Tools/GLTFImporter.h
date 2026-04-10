#pragma once

#include <Ember/Asset/Model.h>
#include <Ember/Asset/Skeleton.h>
#include <Ember/Asset/Animation.h>
#include <Ember/Render/SkinnedMesh.h>

#include <string>

// Forward declaration to avoid including the entire tinygltf header in this file
namespace tinygltf {
	class Model;
}

namespace Ember {

	struct CookedMeshNode
	{
		UUID MeshID;
		uint32_t MaterialIndex;
	};

	struct CookedModelNode
	{
		std::string Name;
		Matrix4f LocalTransform = Matrix4f(1.0f);
		std::vector<CookedModelNode> ChildNodes;
		std::vector<CookedMeshNode> Meshes;
	};

	struct CookedMaterialDef
	{
		std::string BaseMaterial;
		Vector3f AlbedoColor = { 1.0f, 1.0f, 1.0f };
		float Emission = 0.0f;
		float Roughness = 0.5f;
		float Metallic = 0.0f;

		std::string AlbedoMapPath;
		std::string NormalMapPath;
		std::string EmissiveMapPath;
		std::string ORMMapPath;
	};

	struct CookedAssetInfo
	{
		UUID id;
		std::string name;
		std::string path;
	};

	struct ModelCookReport
	{
		CookedAssetInfo Model;
		CookedAssetInfo Skeleton;
		std::vector<CookedAssetInfo> Animations;
		std::vector<CookedAssetInfo> Meshes;
		std::vector<CookedAssetInfo> Materials;
		std::vector<CookedAssetInfo> Textures;
	};

	class GLTFImporter 
	{
	public:
		static std::optional<ModelCookReport> CookModel(const std::string& inputFile, const std::string& outputDirectory);

	private:		
		static CookedModelNode ProcessNode(int nodeIndex, const tinygltf::Model& model, const std::string& outputDirectory,
			const std::vector<std::vector<CookedAssetInfo>>& meshToCookedPrims);

		// Assets
		static std::vector<CookedAssetInfo> ProcessMesh(const std::string& modelName, int meshIndex, const tinygltf::Model& model,
			const std::string& outputDirectory);
		static CookedAssetInfo ProcessMaterial(const std::string& modelName, int matIndex, const tinygltf::Model& model, 
			const std::string& outputDirectory, std::vector<CookedAssetInfo>& cookedImages, bool isSkinned);
	};

}