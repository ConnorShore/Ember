#include "ebpch.h"
#include "ModelImporter.h"
#include "AssetManager.h"

#include "Ember/Render/Renderer3D.h"

#include <filesystem>

namespace Ember {

	unsigned int ModelImporter::m_MeshCounter = 0;

	SharedPtr<Model> ModelImporter::Load(const std::string& name, const std::string& filePath, AssetManager& assetManager)
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(filePath,
			aiProcess_Triangulate |
			aiProcess_FlipUVs |
			aiProcess_JoinIdenticalVertices |
			aiProcess_GenNormals
		);

		std::vector<SharedPtr<MaterialBase>> materials;
		if (scene->HasMaterials())
		{
			materials.reserve(scene->mNumMaterials);
			for (unsigned int i = 0; i < scene->mNumMaterials; i++)
			{
				aiMaterial* material = scene->mMaterials[i];
				materials.push_back(ProcessMaterial(name, filePath, material, assetManager));
			}
		}

		m_MeshCounter = 0;
		auto rootModelNode = ProcessScene(name, scene);
		return SharedPtr<Model>::Create(name, filePath, rootModelNode, materials);
	}

	ModelNode ModelImporter::ProcessScene(const std::string& name, const aiScene* scene)
	{
		ModelNode rootNode;
		rootNode.Name = scene->mRootNode->mName.C_Str();
		rootNode.LocalTransform = ConvertMatrix(scene->mRootNode->mTransformation);
		ProcessNode(name, scene->mRootNode, rootNode, scene);
		return rootNode;
	}

	Matrix4f ModelImporter::ConvertMatrix(const aiMatrix4x4& aiMat)
	{
		return Matrix4f(
			aiMat.a1, aiMat.b1, aiMat.c1, aiMat.d1, // Column 1
			aiMat.a2, aiMat.b2, aiMat.c2, aiMat.d2, // Column 2
			aiMat.a3, aiMat.b3, aiMat.c3, aiMat.d3, // Column 3
			aiMat.a4, aiMat.b4, aiMat.c4, aiMat.d4  // Column 4
		);
	}

	void ModelImporter::ProcessNode(const std::string& name, aiNode* aiNode, ModelNode& modelNode, const aiScene* scene)
	{
		for (unsigned int i = 0; i < aiNode->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[aiNode->mMeshes[i]];
			MeshMaterialNode meshMaterialNode;
			meshMaterialNode.MaterialIndex = mesh->mMaterialIndex;
			meshMaterialNode.MeshAsset = ProcessMesh(name, mesh);
			modelNode.Meshes.push_back(meshMaterialNode);
		}

		modelNode.ChildNodes.reserve(aiNode->mNumChildren);
		for (unsigned int i = 0; i < aiNode->mNumChildren; i++)
		{
			ModelNode childNode;
			childNode.Name = aiNode->mChildren[i]->mName.C_Str();
			childNode.LocalTransform = ConvertMatrix(aiNode->mChildren[i]->mTransformation);
			ProcessNode(name, aiNode->mChildren[i], childNode, scene);
			modelNode.ChildNodes.push_back(childNode);
		}
	}

	SharedPtr<Mesh> ModelImporter::ProcessMesh(const std::string& name, const aiMesh* aiMesh)
	{
		std::vector<float> vertices;
		std::vector<unsigned int> indices;

		vertices.reserve(aiMesh->mNumVertices * 8);
		for (unsigned int i = 0; i < aiMesh->mNumVertices; i++)
		{
			vertices.push_back(aiMesh->mVertices[i].x);
			vertices.push_back(aiMesh->mVertices[i].y);
			vertices.push_back(aiMesh->mVertices[i].z);
			if (aiMesh->mNormals)
			{
				vertices.push_back(aiMesh->mNormals[i].x);
				vertices.push_back(aiMesh->mNormals[i].y);
				vertices.push_back(aiMesh->mNormals[i].z);
			}
			else
			{
				vertices.push_back(0.0f);
				vertices.push_back(0.0f);
				vertices.push_back(0.0f);
			}
			if (aiMesh->mTextureCoords[0])
			{
				vertices.push_back(aiMesh->mTextureCoords[0][i].x);
				vertices.push_back(aiMesh->mTextureCoords[0][i].y);
			}
			else
			{
				vertices.push_back(0.0f);
				vertices.push_back(0.0f);
			}
		}
		for (unsigned int i = 0; i < aiMesh->mNumFaces; i++)
		{
			for (unsigned int j = 0; j < aiMesh->mFaces[i].mNumIndices; j++)
			{
				indices.push_back(aiMesh->mFaces[i].mIndices[j]);
			}
		}

		std::string meshName = name + "_Mesh_" + std::to_string(m_MeshCounter++);
		return SharedPtr<Mesh>::Create(meshName, vertices, indices);
	}

	SharedPtr<MaterialInstance> ModelImporter::ProcessMaterial(const std::string& modelName, const std::string& modelFilePath, const aiMaterial* aiMat, AssetManager& assetManager)
	{
		std::string rawMatName = aiMat->GetName().C_Str();
		std::string matName = modelName + "_" + rawMatName;

		// Figure out what kind of shader this needs
		std::string baseMatName = DetermineBaseMaterial(aiMat);

		auto matInstance = SharedPtr<MaterialInstance>::Create(matName, assetManager.GetAsset<Material>(baseMatName));

		// Populate uniform overrides
		ExtractPBRUniforms(aiMat, matInstance, baseMatName);
		ExtractTextures(matName, modelFilePath, aiMat, matInstance, assetManager);

		return matInstance;
	}

	std::string ModelImporter::DetermineBaseMaterial(const aiMaterial* aiMat)
	{
		std::string rawMatName = aiMat->GetName().C_Str();
		std::string baseMatName = Constants::Assets::StandardGeometryMat;

		aiColor4D emissiveColor(0.0f, 0.0f, 0.0f, 1.0f);
		if (AI_SUCCESS == aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_EMISSIVE, &emissiveColor))
		{
			if (emissiveColor.r > 0.0f || emissiveColor.g > 0.0f || emissiveColor.b > 0.0f)
				return Constants::Assets::StandardUnlitMat;
		}

		float opacity = 1.0f;
		if (AI_SUCCESS == aiGetMaterialFloat(aiMat, AI_MATKEY_OPACITY, &opacity))
		{
			if (opacity < 1.0f)
				return Constants::Assets::StandardUnlitMat;
		}

		aiShadingMode shadingMode;
		if (AI_SUCCESS == aiMat->Get(AI_MATKEY_SHADING_MODEL, shadingMode))
		{
			if (shadingMode == aiShadingMode_NoShading)
				return Constants::Assets::StandardUnlitMat;
		}

		if (rawMatName.find("_Unlit") != std::string::npos || rawMatName.find("_Forward") != std::string::npos)
			return Constants::Assets::StandardUnlitMat;
		else if (rawMatName.find("_Opaque") != std::string::npos || rawMatName.find("_Deferred") != std::string::npos)
			return Constants::Assets::StandardGeometryMat;

		return baseMatName;
	}

	void ModelImporter::ExtractPBRUniforms(const aiMaterial* aiMat, SharedPtr<MaterialInstance>& matInstance, const std::string& baseMatName)
	{
		aiColor4D diffuseColor(1.0f, 1.0f, 1.0f, 1.0f);
		aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_DIFFUSE, &diffuseColor);
		Ember::Vector3f finalColor(diffuseColor.r, diffuseColor.g, diffuseColor.b);

		aiColor4D emissiveColor(0.0f, 0.0f, 0.0f, 1.0f);
		aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_EMISSIVE, &emissiveColor);

		if (baseMatName == Constants::Assets::StandardUnlitMat && (emissiveColor.r > 0.0f || emissiveColor.g > 0.0f || emissiveColor.b > 0.0f))
		{
			finalColor = Ember::Vector3f(emissiveColor.r, emissiveColor.g, emissiveColor.b);
		}

		matInstance->Set(Constants::Uniforms::Albedo, finalColor);
		matInstance->Set(Constants::Uniforms::Color, finalColor);

		float roughness = 0.5f;
		aiGetMaterialFloat(aiMat, AI_MATKEY_ROUGHNESS_FACTOR, &roughness);
		matInstance->Set(Constants::Uniforms::Roughness, roughness);

		float metallic = 0.0f;
		aiGetMaterialFloat(aiMat, AI_MATKEY_METALLIC_FACTOR, &metallic);
		matInstance->Set(Constants::Uniforms::Metallic, metallic);

		matInstance->Set(Constants::Uniforms::AO, 1.0f);
	}

	void ModelImporter::ExtractTextures(const std::string& matName, const std::string& modelFilePath, const aiMaterial* aiMat, SharedPtr<MaterialInstance>& matInstance, AssetManager& assetManager)
	{
		if (aiMat->GetTextureCount(aiTextureType_DIFFUSE) > 0)
		{
			aiString texPath;
			aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath);

			std::filesystem::path modelPath(modelFilePath);
			std::filesystem::path directory = modelPath.parent_path();
			std::filesystem::path fullTexPath = directory / texPath.C_Str();
			std::string finalPath = fullTexPath.generic_string();

			std::string texName = matName + "_AlbedoMap";
			auto albedoMap = assetManager.Load<Texture>(texName, finalPath);
			matInstance->Set(Constants::Uniforms::AlbedoMap, albedoMap);
		}
		else
		{
			matInstance->Set(Constants::Uniforms::AlbedoMap, assetManager.GetAsset<Texture>(Constants::Assets::DefaultWhiteTex));
		}

		// TODO: Later on, this is where you will add:
		// if (aiMat->GetTextureCount(aiTextureType_NORMALS) > 0) { ... load NormalMap ... }
	}

}