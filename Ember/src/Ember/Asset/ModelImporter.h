#pragma once

#include "Model.h"
#include "Ember/Render/Mesh.h"
#include "Ember/Core/Core.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <string>

namespace Ember {

	class AssetManager;

	class ModelImporter
	{
	public:// Update the signature to accept optional UUID arrays
		static SharedPtr<Model> Load(UUID uuid, const std::string& name, const std::string& filePath, AssetManager& assetManager,
			const std::vector<UUID>& meshUUIDs = {}, const std::vector<UUID>& materialUUIDs = {});
		static SharedPtr<Model> Load(const std::string& name, const std::string& filePath, AssetManager& assetManager);

	private:
		static unsigned int m_MeshCounter;

		static ModelNode ProcessScene(const std::string& name, const aiScene* scene, AssetManager& assetManager, const std::vector<UUID>& meshUUIDs);
		static Matrix4f ConvertMatrix(const aiMatrix4x4& aiMat);
		static void ProcessNode(const std::string& name, aiNode* aiNode, ModelNode& modelNode, const aiScene* scene, AssetManager& assetManager, const std::vector<UUID>& meshUUIDs);
		static SharedPtr<Mesh> ProcessMesh(const std::string& name, const aiMesh* aiMesh, AssetManager& assetManager, const std::vector<UUID>& meshUUIDs);
		static SharedPtr<MaterialInstance> ProcessMaterial(const std::string& modelName, const std::string& modelFilePath, const aiScene* scene, const aiMaterial* aiMat, AssetManager& assetManager);

		static std::string DetermineBaseMaterial(const aiMaterial* aiMat);
		static void ExtractPBRUniforms(const aiMaterial* aiMat, SharedPtr<MaterialInstance>& matInstance, const std::string& baseMatName);
		static void ExtractTextures(const std::string& matName, const std::string& modelFilePath, const aiScene* scene, const aiMaterial* aiMat, SharedPtr<MaterialInstance>& matInstance, AssetManager& assetManager);
		static void LoadTextureToUniform(const std::string& matName, const std::string& modelFilePath, const aiScene* scene, const aiMaterial* aiMat, const std::vector<aiTextureType>& typesToCheck, const std::string& texNameSuffix, const std::string& uniformName, const std::string& fallbackTex, SharedPtr<MaterialInstance>& matInstance, AssetManager& assetManager);
	};

}