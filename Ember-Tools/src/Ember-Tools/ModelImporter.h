#pragma once

#include <Ember.h>
#include <string>
#include <vector>
#include <filesystem>
#include <optional>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace Ember {

	struct CookedMeshNode
	{
		UUID MeshID;
		unsigned int MaterialIndex;
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
		std::vector<CookedAssetInfo> Meshes;
		std::vector<CookedAssetInfo> Materials;
		std::vector<CookedAssetInfo> Textures;
	};

	class ModelImporter
	{
	public:
		static std::optional<ModelCookReport> CookModel(const std::string& inputFile, const std::string& outputDirectory);
		static bool CookMesh(const std::vector<MeshVertex>& vertices, const std::vector<uint32_t>& indices, const std::string& outputFilePath);

	private:
		static CookedModelNode ProcessScene(const std::string& name, const aiScene* scene, const std::string& outputDirectory, const std::vector<CookedAssetInfo>& cookedMeshes);
		static Matrix4f ConvertMatrix(const aiMatrix4x4& aiMat);
		static void ProcessNode(const std::string& name, aiNode* aiNode, CookedModelNode& modelNode, const aiScene* scene, const std::string& outputDirectory, const std::vector<CookedAssetInfo>& cookedMeshes);

		static CookedAssetInfo ProcessMesh(const std::string& name, const aiMesh* aiMesh, const std::string& outputDirectory);
		static CookedAssetInfo ProcessMaterial(const std::string& modelName, const std::string& modelFilePath, const aiScene* scene, const aiMaterial* aiMat, const std::string& outputDirectory, std::vector<CookedAssetInfo>& outTextures);
		static CookedAssetInfo ExtractAndCopyTexture(const std::string& matName, const std::string& modelFilePath, const aiScene* scene, const aiMaterial* aiMat, const std::vector<aiTextureType>& typesToCheck, const std::string& texNameSuffix, const std::string& outputDirectory);
		
		static std::string DetermineBaseMaterial(const aiMaterial* aiMat);
		static void ExtractPBRUniforms(const aiMaterial* aiMat, CookedMaterialDef& def);
		static void ExtractTextures(const std::string& matName, const std::string& modelFilePath, const aiScene* scene, const aiMaterial* aiMat, CookedMaterialDef& def, const std::string& outputDirectory, std::vector<CookedAssetInfo>& outTextures);
	};

}