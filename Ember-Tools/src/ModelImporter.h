#pragma once

#include <Ember.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <string>

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

	class ModelImporter
	{
	public:
		static bool CookModel(const std::string& inputFile, const std::string& outputDirectory);
		static bool CookMesh(const std::vector<MeshVertex>& vertices, const std::vector<uint32_t>& indices, const std::string& outputFilePath);

	private:
		static unsigned int m_MeshCounter;

		static CookedModelNode ProcessScene(const std::string& name, const aiScene* scene, const std::string& outputDirectory);
		static Matrix4f ConvertMatrix(const aiMatrix4x4& aiMat);
		static void ProcessNode(const std::string& name, aiNode* aiNode, CookedModelNode& modelNode, const aiScene* scene, const std::string& outputDirectory);
		static UUID ProcessMesh(const std::string& name, const aiMesh* aiMesh, const std::string& outputDirectory);
		static SharedPtr<MaterialInstance> ProcessMaterial(const std::string& modelName, const std::string& modelFilePath, const aiScene* scene, const aiMaterial* aiMat, AssetManager& assetManager);

		static std::string DetermineBaseMaterial(const aiMaterial* aiMat);
		static void ExtractPBRUniforms(const aiMaterial* aiMat, SharedPtr<MaterialInstance>& matInstance, const std::string& baseMatName);
		static void ExtractTextures(const std::string& matName, const std::string& modelFilePath, const aiScene* scene, const aiMaterial* aiMat, SharedPtr<MaterialInstance>& matInstance, AssetManager& assetManager);
		static void LoadTextureToUniform(const std::string& matName, const std::string& modelFilePath, const aiScene* scene, const aiMaterial* aiMat, const std::vector<aiTextureType>& typesToCheck, const std::string& texNameSuffix, const std::string& uniformName, const std::string& fallbackTex, SharedPtr<MaterialInstance>& matInstance, AssetManager& assetManager);
	};

}