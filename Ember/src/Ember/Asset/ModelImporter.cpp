#include "ebpch.h"
#include "ModelImporter.h"
#include "AssetManager.h"

#include "Ember/Render/Renderer3D.h"

#include <filesystem>
#include <stb_image.h>
#include <stb_image_write.h>

namespace Ember {

	unsigned int ModelImporter::m_MeshCounter = 0;

	SharedPtr<Model> ModelImporter::Load(UUID uuid, const std::string& name, const std::string& filePath, AssetManager& assetManager,
		const std::vector<UUID>& meshUUIDs /*= {}*/, const std::vector<UUID>& materialUUIDs /*= {}*/)
	{
		Assimp::Importer importer;

		unsigned int importFlags =
			aiProcess_Triangulate |
			aiProcess_FlipUVs |
			aiProcess_JoinIdenticalVertices |
			aiProcess_GenNormals |
			aiProcess_CalcTangentSpace;

		std::string extension = std::filesystem::path(filePath).extension().string();
		if (extension != ".glb" && extension != ".gltf")
		{
			importFlags |= aiProcess_FlipUVs;
		}

		const aiScene* scene = importer.ReadFile(filePath, importFlags);

		std::vector<SharedPtr<MaterialBase>> materials;
		if (scene->HasMaterials())
		{
			materials.reserve(scene->mNumMaterials);
			for (unsigned int i = 0; i < scene->mNumMaterials; i++)
			{
				if (i < materialUUIDs.size())
				{
					materials.push_back(assetManager.GetAsset<MaterialBase>(materialUUIDs[i]));
				}
				else
				{
					// We are importing for the very first time. Extract from Assimp.
					aiMaterial* material = scene->mMaterials[i];
					materials.push_back(ProcessMaterial(name, filePath, scene, material, assetManager));
				}
			}
		}

		m_MeshCounter = 0;
		auto rootModelNode = ProcessScene(name, scene, assetManager, meshUUIDs);
		return SharedPtr<Model>::Create(uuid, name, filePath, rootModelNode, materials);
	}

	SharedPtr<Model> ModelImporter::Load(const std::string& name, const std::string& filePath, AssetManager& assetManager)
	{
		return Load(UUID(), name, filePath, assetManager);
	}

	ModelNode ModelImporter::ProcessScene(const std::string& name, const aiScene* scene, AssetManager& assetManager, const std::vector<UUID>& meshUUIDs)
	{
		ModelNode rootNode;
		rootNode.Name = scene->mRootNode->mName.C_Str();
		rootNode.LocalTransform = ConvertMatrix(scene->mRootNode->mTransformation);
		ProcessNode(name, scene->mRootNode, rootNode, scene, assetManager, meshUUIDs);
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

	void ModelImporter::ProcessNode(const std::string& name, aiNode* aiNode, ModelNode& modelNode, const aiScene* scene, AssetManager& assetManager, const std::vector<UUID>& meshUUIDs)
	{
		for (unsigned int i = 0; i < aiNode->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[aiNode->mMeshes[i]];
			MeshMaterialNode meshMaterialNode;
			meshMaterialNode.MaterialIndex = mesh->mMaterialIndex;
			meshMaterialNode.MeshAsset = ProcessMesh(name, mesh, assetManager, meshUUIDs);
			modelNode.Meshes.push_back(meshMaterialNode);
		}

		modelNode.ChildNodes.reserve(aiNode->mNumChildren);
		for (unsigned int i = 0; i < aiNode->mNumChildren; i++)
		{
			ModelNode childNode;
			childNode.Name = aiNode->mChildren[i]->mName.C_Str();
			childNode.LocalTransform = ConvertMatrix(aiNode->mChildren[i]->mTransformation);
			ProcessNode(name, aiNode->mChildren[i], childNode, scene, assetManager, meshUUIDs);
			modelNode.ChildNodes.push_back(childNode);
		}
	}

	SharedPtr<Mesh> ModelImporter::ProcessMesh(const std::string& name, const aiMesh* aiMesh, AssetManager& assetManager, const std::vector<UUID>& meshUUIDs)
	{
		std::vector<float> vertices;
		std::vector<unsigned int> indices;

		static unsigned int vertexSize = 14; // Position(3) + Normal(3) + TexCoords(2) + Tangent(3) + Bitangent(3)
		vertices.reserve(aiMesh->mNumVertices * vertexSize);

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
			if (aiMesh->mTangents)
			{
				vertices.push_back(aiMesh->mTangents[i].x);
				vertices.push_back(aiMesh->mTangents[i].y);
				vertices.push_back(aiMesh->mTangents[i].z);
			}
			else
			{
				vertices.push_back(1.0f);
				vertices.push_back(0.0f);
				vertices.push_back(0.0f);
			}
			if (aiMesh->mBitangents)
			{
				vertices.push_back(aiMesh->mBitangents[i].x);
				vertices.push_back(aiMesh->mBitangents[i].y);
				vertices.push_back(aiMesh->mBitangents[i].z);
			}
			else
			{
				vertices.push_back(0.0f);
				vertices.push_back(1.0f);
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

		std::string meshName = name + "_" + aiMesh->mName.C_Str();
		UUID currentMeshUUID = UUID();
		if (m_MeshCounter < meshUUIDs.size())
		{
			currentMeshUUID = meshUUIDs[m_MeshCounter];
		}
		m_MeshCounter++;

		auto ret = SharedPtr<Mesh>::Create(currentMeshUUID, meshName, vertices, indices);
		assetManager.Register(ret);
		return ret;
	}

	SharedPtr<MaterialInstance> ModelImporter::ProcessMaterial(const std::string& modelName, const std::string& modelFilePath, const aiScene* scene, const aiMaterial* aiMat, AssetManager& assetManager)
	{
		std::string rawMatName = aiMat->GetName().C_Str();
		std::string matName = modelName + "_" + rawMatName;

		// Figure out what kind of shader this needs
		std::string baseMatName = DetermineBaseMaterial(aiMat);

		auto matInstance = SharedPtr<MaterialInstance>::Create(matName, assetManager.GetAsset<Material>(baseMatName));

		// Populate uniform overrides
		ExtractPBRUniforms(aiMat, matInstance, baseMatName);
		ExtractTextures(matName, modelFilePath, scene, aiMat, matInstance, assetManager);

		assetManager.Register(matInstance);

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
		if (AI_SUCCESS != aiGetMaterialColor(aiMat, AI_MATKEY_BASE_COLOR, &diffuseColor))
		{
			aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_DIFFUSE, &diffuseColor);
		}
		Ember::Vector3f finalColor(diffuseColor.r, diffuseColor.g, diffuseColor.b);

		aiColor4D emissiveColor(0.0f, 0.0f, 0.0f, 1.0f);
		aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_EMISSIVE, &emissiveColor);

		float emissionIntensity = 0.0f;
		if (baseMatName == Constants::Assets::StandardUnlitMat)
		{
			// Find the brightest color channel
			float maxEmission = std::max({ emissiveColor.r, emissiveColor.g, emissiveColor.b });
			if (maxEmission > 0.0f)
			{
				// If the 3D software exported HDR values (e.g., R=5.0)
				// We extract 5.0 as the intensity, and normalize the color back to 1.0!
				if (maxEmission > 1.0f)
				{
					emissionIntensity = maxEmission;
					finalColor = Ember::Vector3f(emissiveColor.r / maxEmission, emissiveColor.g / maxEmission, emissiveColor.b / maxEmission);
				}
				else
				{
					// Standard 0.0 to 1.0 range emission
					emissionIntensity = 1.0f;
					finalColor = Ember::Vector3f(emissiveColor.r, emissiveColor.g, emissiveColor.b);
				}
			}
			else
			{
				// It's an unlit shader but has no emissive map, fallback to diffuse color with 1.0 intensity
				emissionIntensity = 1.0f;
			}
		}

		matInstance->SetUniform(Constants::Uniforms::Albedo, finalColor);
		matInstance->SetUniform(Constants::Uniforms::Color, finalColor);
		matInstance->SetUniform(Constants::Uniforms::Emission, emissionIntensity);

		float roughness = 0.5f;
		aiGetMaterialFloat(aiMat, AI_MATKEY_ROUGHNESS_FACTOR, &roughness);
		matInstance->SetUniform(Constants::Uniforms::Roughness, roughness);

		float metallic = 0.0f;
		aiGetMaterialFloat(aiMat, AI_MATKEY_METALLIC_FACTOR, &metallic);
		matInstance->SetUniform(Constants::Uniforms::Metallic, metallic);

		matInstance->SetUniform(Constants::Uniforms::AO, 1.0f);
	}

	void ModelImporter::ExtractTextures(const std::string& matName, const std::string& modelFilePath, const aiScene* scene, const aiMaterial* aiMat, SharedPtr<MaterialInstance>& matInstance, AssetManager& assetManager)
	{
		// =========================================================================
		// 1. ALBEDO / BASE COLOR
		// =========================================================================
		aiTextureType albedoType = aiTextureType_NONE;
		if (aiMat->GetTextureCount(aiTextureType_BASE_COLOR) > 0)
			albedoType = aiTextureType_BASE_COLOR; // Modern .glb PBR
		else if (aiMat->GetTextureCount(aiTextureType_DIFFUSE) > 0)
			albedoType = aiTextureType_DIFFUSE; // Legacy .obj

		if (albedoType != aiTextureType_NONE)
		{
			aiString texPath;
			aiMat->GetTexture(albedoType, 0, &texPath);
			std::string texName = matName + "_AlbedoMap";

			const aiTexture* embeddedTexture = scene->GetEmbeddedTexture(texPath.C_Str());

			if (embeddedTexture)
			{
				// We want to save the raw PNG exactly as it is in the GLB to the hard drive. 
				// Our Texture Importer will flip it naturally when it loads the new file.
				stbi_set_flip_vertically_on_load(false);

				int width, height, channels;
				unsigned char* image_data = nullptr;

				// Check if the embedded texture is compressed (PNG/JPG) or raw pixels
				if (embeddedTexture->mHeight == 0)
				{
					image_data = stbi_load_from_memory(
						reinterpret_cast<unsigned char*>(embeddedTexture->pcData),
						embeddedTexture->mWidth,
						&width, &height, &channels, 4);
				}
				else
				{
					width = embeddedTexture->mWidth;
					height = embeddedTexture->mHeight;
					image_data = reinterpret_cast<unsigned char*>(embeddedTexture->pcData);
				}

				if (image_data)
				{
					// Generate a file path right next to the model file
					std::filesystem::path modelPath(modelFilePath);
					std::filesystem::path directory = modelPath.parent_path();
					std::filesystem::path outTexPath = directory / (texName + ".png");

					// Write the embedded texture to the hard drive if it hasn't been extracted yet
					if (!std::filesystem::exists(outTexPath))
					{
						EB_CORE_TRACE("Extracting embedded Albedo texture to: {}", outTexPath.string());
						stbi_write_png(outTexPath.string().c_str(), width, height, 4, image_data, width * 4);
					}

					// Now load it through the Asset Manager like a normal file! (This fixes serialization)
					auto albedoMap = assetManager.Load<Texture>(texName, outTexPath.string());
					matInstance->SetUniform(Constants::Uniforms::AlbedoMap, albedoMap);

					if (embeddedTexture->mHeight == 0)
						stbi_image_free(image_data);
				}
				else
				{
					EB_CORE_ERROR("Failed to decode embedded Albedo texture!");
					matInstance->SetUniform(Constants::Uniforms::AlbedoMap, assetManager.GetAsset<Texture>(Constants::Assets::DefaultWhiteTex));
				}
			}
			else
			{
				// External texture on disk (e.g. from an .obj)
				std::filesystem::path modelPath(modelFilePath);
				std::filesystem::path directory = modelPath.parent_path();
				std::filesystem::path fullTexPath = directory / texPath.C_Str();

				auto albedoMap = assetManager.Load<Texture>(texName, fullTexPath.generic_string());
				matInstance->SetUniform(Constants::Uniforms::AlbedoMap, albedoMap);
			}
		}
		else
		{
			// No texture found, use engine fallback
			matInstance->SetUniform(Constants::Uniforms::AlbedoMap, assetManager.GetAsset<Texture>(Constants::Assets::DefaultWhiteTex));
		}

		// =========================================================================
		// 2. NORMAL MAPS
		// =========================================================================
		aiTextureType normalType = aiTextureType_NONE;
		if (aiMat->GetTextureCount(aiTextureType_NORMALS) > 0)
			normalType = aiTextureType_NORMALS;
		else if (aiMat->GetTextureCount(aiTextureType_HEIGHT) > 0)
			normalType = aiTextureType_HEIGHT;

		if (normalType != aiTextureType_NONE)
		{
			aiString texPath;
			aiMat->GetTexture(normalType, 0, &texPath);
			std::string texName = matName + "_NormalMap";

			const aiTexture* embeddedTexture = scene->GetEmbeddedTexture(texPath.C_Str());

			if (embeddedTexture)
			{
				stbi_set_flip_vertically_on_load(false);

				int width, height, channels;
				unsigned char* image_data = nullptr;

				if (embeddedTexture->mHeight == 0)
				{
					image_data = stbi_load_from_memory(
						reinterpret_cast<unsigned char*>(embeddedTexture->pcData),
						embeddedTexture->mWidth,
						&width, &height, &channels, 4);
				}
				else
				{
					width = embeddedTexture->mWidth;
					height = embeddedTexture->mHeight;
					image_data = reinterpret_cast<unsigned char*>(embeddedTexture->pcData);
				}

				if (image_data)
				{
					std::filesystem::path modelPath(modelFilePath);
					std::filesystem::path directory = modelPath.parent_path();
					std::filesystem::path outTexPath = directory / (texName + ".png");

					if (!std::filesystem::exists(outTexPath))
					{
						EB_CORE_TRACE("Extracting embedded Normal texture to: {}", outTexPath.string());
						stbi_write_png(outTexPath.string().c_str(), width, height, 4, image_data, width * 4);
					}

					auto normalMap = assetManager.Load<Texture>(texName, outTexPath.string());
					matInstance->SetUniform(Constants::Uniforms::NormalMap, normalMap);

					if (embeddedTexture->mHeight == 0)
						stbi_image_free(image_data);
				}
				else
				{
					EB_CORE_ERROR("Failed to decode embedded Normal texture!");
					matInstance->SetUniform(Constants::Uniforms::NormalMap, assetManager.GetAsset<Texture>(Constants::Assets::DefaultNormalTex));
				}
			}
			else
			{
				std::filesystem::path modelPath(modelFilePath);
				std::filesystem::path directory = modelPath.parent_path();
				std::filesystem::path fullTexPath = directory / texPath.C_Str();

				auto normalMap = assetManager.Load<Texture>(texName, fullTexPath.generic_string());
				matInstance->SetUniform(Constants::Uniforms::NormalMap, normalMap);
			}
		}
		else
		{
			matInstance->SetUniform(Constants::Uniforms::NormalMap, assetManager.GetAsset<Texture>(Constants::Assets::DefaultNormalTex));
		}
	}

}