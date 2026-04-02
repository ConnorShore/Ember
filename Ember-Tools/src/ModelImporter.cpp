#include "ModelImporter.h"

#include <Ember/Asset/MeshHeader.h>

#include <filesystem>
#include <fstream>
#include <ryml.hpp>

namespace Ember {

	unsigned int ModelImporter::m_MeshCounter = 0;

	bool ModelImporter::CookModel(const std::string& inputFile, const std::string& outputDirectory)
	{
		// ... [Assimp import and ProcessScene logic] ...

		std::string modelName = std::filesystem::path(inputFile).stem().string();
		std::filesystem::path manifestPath = std::filesystem::path(outputDirectory) / (modelName + ".ebmodel");

		// --- ryml Tree Construction ---
		ryml::Tree tree;
		ryml::NodeRef root = tree.rootref();
		root |= ryml::MAP; // Set root as a map

		root["Model"] << modelName;

		// RootNode Map
		ryml::NodeRef rootNodeRef = root["RootNode"];
		rootNodeRef |= ryml::MAP;
		rootNodeRef["Name"] << rootNode.Name;

		// TODO: Serialize your Matrix4f Transform here

		// Meshes Sequence
		ryml::NodeRef meshesRef = rootNodeRef["Meshes"];
		meshesRef |= ryml::SEQ;

		for (const auto& meshNode : rootNode.Meshes)
		{
			ryml::NodeRef meshMap = meshesRef.append_child();
			meshMap |= ryml::MAP;

			// Cast UUID to uint64_t so ryml knows how to format it as a number
			meshMap["MeshID"] << (uint64_t)meshNode.MeshID;
			meshMap["MaterialIndex"] << meshNode.MaterialIndex;
		}

		// --- Emit and Write ---
		// ryml::emitrs_yaml emits the tree directly to an std::string
		std::string yamlOutput = ryml::emitrs_yaml<std::string>(tree);

		std::ofstream fout(manifestPath);
		fout << yamlOutput;
		fout.close();

		return true;
	}

	bool ModelImporter::CookMesh(const std::vector<MeshVertex>& vertices, const std::vector<uint32_t>& indices, const std::string& outputFilePath)
	{
		// Open the output file in binary mode
		std::ofstream file(outputFilePath, std::ios::binary | std::ios::trunc);
		if (!file.is_open())
		{
			EB_CORE_ERROR("Failed to open file for cooking: {}", outputFilePath);
			return false;
		}

		// Write the Mesh Header
		MeshHeader header;
		header.VertexCount = static_cast<uint32_t>(vertices.size());
		header.IndexCount = static_cast<uint32_t>(indices.size());

		// Calculate Bounding Box
		Vector3f minBounds(FLT_MAX);
		Vector3f maxBounds(-FLT_MAX);
		for (const auto& v : vertices)
		{
			minBounds = Math::Min(minBounds, v.Position);
			maxBounds = Math::Max(maxBounds, v.Position);
		}

		header.Bounds.Min[0] = minBounds.x; header.Bounds.Min[1] = minBounds.y; header.Bounds.Min[2] = minBounds.z;
		header.Bounds.Max[0] = maxBounds.x; header.Bounds.Max[1] = maxBounds.y; header.Bounds.Max[2] = maxBounds.z;

		// Write the header
		file.write((const char*)&header, sizeof(MeshHeader));

		// Write vertices and indices
		size_t vertexDataSize = vertices.size() * sizeof(MeshVertex);
		file.write((const char*)vertices.data(), vertexDataSize);

		size_t indexDataSize = indices.size() * sizeof(uint32_t);
		file.write((const char*)indices.data(), indexDataSize);

		file.close();

		EB_CORE_INFO("Successfully cooked mesh to {}!", outputFilePath);
		return true;
	}

	UUID ModelImporter::ProcessMesh(const std::string& name, const aiMesh* aiMesh, const std::string& outputDirectory)
	{
		std::vector<MeshVertex> vertices;
		std::vector<unsigned int> indices;

		vertices.reserve(aiMesh->mNumVertices);

		for (unsigned int i = 0; i < aiMesh->mNumVertices; i++)
		{
			MeshVertex vertex;

			// Position
			vertex.Position = { aiMesh->mVertices[i].x, aiMesh->mVertices[i].y, aiMesh->mVertices[i].z };

			// Normal
			if (aiMesh->mNormals)
			{
				vertex.Normal = { aiMesh->mNormals[i].x, aiMesh->mNormals[i].y, aiMesh->mNormals[i].z };
			}
			else
			{
				vertex.Normal = { 0.0f, 0.0f, 0.0f };
			}

			// Texture Coordinates
			if (aiMesh->mTextureCoords[0])
			{
				vertex.TexCoords = { aiMesh->mTextureCoords[0][i].x, aiMesh->mTextureCoords[0][i].y };
			}
			else
			{
				vertex.TexCoords = { 0.0f, 0.0f };
			}

			// Tangent
			if (aiMesh->mTangents)
			{
				vertex.Tangent = { aiMesh->mTangents[i].x, aiMesh->mTangents[i].y, aiMesh->mTangents[i].z };
			}
			else
			{
				vertex.Tangent = { 1.0f, 0.0f, 0.0f };
			}

			// Bitangent
			if (aiMesh->mBitangents)
			{
				vertex.Bitangent = { aiMesh->mBitangents[i].x, aiMesh->mBitangents[i].y, aiMesh->mBitangents[i].z };
			}
			else
			{
				vertex.Bitangent = { 0.0f, 1.0f, 0.0f };
			}

			vertices.push_back(vertex);
		}

		// Indices
		for (unsigned int i = 0; i < aiMesh->mNumFaces; i++)
		{
			for (unsigned int j = 0; j < aiMesh->mFaces[i].mNumIndices; j++)
			{
				indices.push_back(aiMesh->mFaces[i].mIndices[j]);
			}
		}

		// Cook the mesh
		UUID meshUUID = UUID();
		std::string meshFileName = name + "_" + aiMesh->mName.C_Str() + ".ebmesh";
		std::filesystem::path outputPath = std::filesystem::path(outputDirectory) / meshFileName;

		// Let CookMesh do the heavy lifting
		if (CookMesh(vertices, indices, outputPath.string()))
		{
			return meshUUID;
		}

		EB_CORE_ERROR("Failed to cook mesh: {0}", meshFileName);
		return Constants::InvalidUUID;
	}

	SharedPtr<MaterialInstance> ModelImporter::ProcessMaterial(const std::string& modelName, const std::string& modelFilePath, const aiScene* scene, const aiMaterial* aiMat, AssetManager& assetManager)
	{
		std::string rawMatName = aiMat->GetName().C_Str();
		std::string matName = modelName + "_" + rawMatName;

		// Figure out what kind of shader this needs
		std::string baseMatName = DetermineBaseMaterial(aiMat);

		auto matInstance = SharedPtr<MaterialInstance>::Create(matName, assetManager.GetAsset<Material>(baseMatName));
		matInstance->SetIsEngineAsset(false);

		// Populate uniform overrides
		ExtractPBRUniforms(aiMat, matInstance, baseMatName);
		ExtractTextures(matName, modelFilePath, scene, aiMat, matInstance, assetManager);

		assetManager.Register(matInstance);

		return matInstance;
	}

	std::string ModelImporter::DetermineBaseMaterial(const aiMaterial* aiMat)
	{
		std::string rawMatName = aiMat->GetName().C_Str();

		// Default to standard PBR geometry
		std::string baseMatName = Constants::Assets::StandardGeometryMat;

		// Only use Unlit if the model explicitly requests no shading
		aiShadingMode shadingMode;
		if (AI_SUCCESS == aiMat->Get(AI_MATKEY_SHADING_MODEL, shadingMode))
		{
			if (shadingMode == aiShadingMode_NoShading)
				return Constants::Assets::StandardUnlitMat;
		}

		// Fallback name-based checks
		if (rawMatName.find("_Unlit") != std::string::npos || rawMatName.find("_Forward") != std::string::npos)
			return Constants::Assets::StandardUnlitMat;
		else if (rawMatName.find("_Opaque") != std::string::npos || rawMatName.find("_Deferred") != std::string::npos)
			return Constants::Assets::StandardGeometryMat;

		return baseMatName;
	}

	void ModelImporter::ExtractPBRUniforms(const aiMaterial* aiMat, SharedPtr<MaterialInstance>& matInstance, const std::string& baseMatName)
	{
		// Base Color / Albedo
		aiColor4D diffuseColor(1.0f, 1.0f, 1.0f, 1.0f);
		if (AI_SUCCESS != aiGetMaterialColor(aiMat, AI_MATKEY_BASE_COLOR, &diffuseColor))
		{
			aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_DIFFUSE, &diffuseColor);
		}
		Ember::Vector3f finalColor(diffuseColor.r, diffuseColor.g, diffuseColor.b);

		// Emission
		aiColor4D emissiveColor(0.0f, 0.0f, 0.0f, 1.0f);
		aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_EMISSIVE, &emissiveColor);

		float emissionIntensity = 0.0f;
		float maxEmission = std::max({ emissiveColor.r, emissiveColor.g, emissiveColor.b });

		if (baseMatName == Constants::Assets::StandardUnlitMat)
		{
			// UNLIT LOGIC: Emissive color overrides the Base Color
			if (maxEmission > 0.0f)
			{
				if (maxEmission > 1.0f)
				{
					emissionIntensity = maxEmission;
					finalColor = Ember::Vector3f(emissiveColor.r / maxEmission, emissiveColor.g / maxEmission, emissiveColor.b / maxEmission);
				}
				else
				{
					emissionIntensity = 1.0f;
					finalColor = Ember::Vector3f(emissiveColor.r, emissiveColor.g, emissiveColor.b);
				}
			}
			else
			{
				emissionIntensity = 1.0f; // Fallback to diffuse color with 1.0 intensity
			}
		}
		else
		{
			// DEFERRED PBR LOGIC (StandardGeometryMat)
			emissionIntensity = maxEmission;

			// Safety check: If the color factor is 0, but an artist attached an Emissive Texture,
			// force the intensity to 1.0 so the texture is actually visible!
			if (emissionIntensity == 0.0f && aiMat->GetTextureCount(aiTextureType_EMISSIVE) > 0)
			{
				emissionIntensity = 1.0f;
			}
		}

		matInstance->SetUniform(Constants::Uniforms::Albedo, finalColor);
		matInstance->SetUniform(Constants::Uniforms::Color, finalColor);
		matInstance->SetUniform(Constants::Uniforms::Emission, emissionIntensity);

		// Roughness & Metallic
		float roughness = 0.5f;
		aiGetMaterialFloat(aiMat, AI_MATKEY_ROUGHNESS_FACTOR, &roughness);
		matInstance->SetUniform(Constants::Uniforms::Roughness, roughness);

		float metallic = 0.0f;
		aiGetMaterialFloat(aiMat, AI_MATKEY_METALLIC_FACTOR, &metallic);
		matInstance->SetUniform(Constants::Uniforms::Metallic, metallic);

		// Ambient Occlusion
		matInstance->SetUniform(Constants::Uniforms::AO, 1.0f);
	}
	void ModelImporter::ExtractTextures(const std::string& matName, const std::string& modelFilePath, const aiScene* scene, const aiMaterial* aiMat, SharedPtr<MaterialInstance>& matInstance, AssetManager& assetManager)
	{
		// Albedo/Diffuse
		LoadTextureToUniform(matName, modelFilePath, scene, aiMat,
			{ aiTextureType_BASE_COLOR, aiTextureType_DIFFUSE },
			"_AlbedoMap", Constants::Uniforms::AlbedoMap, Constants::Assets::DefaultWhiteTex, matInstance, assetManager);

		// Normal
		LoadTextureToUniform(matName, modelFilePath, scene, aiMat,
			{ aiTextureType_NORMALS, aiTextureType_HEIGHT },
			"_NormalMap", Constants::Uniforms::NormalMap, Constants::Assets::DefaultNormalTex, matInstance, assetManager);

		// Emissive
		LoadTextureToUniform(matName, modelFilePath, scene, aiMat,
			{ aiTextureType_EMISSIVE, aiTextureType_EMISSION_COLOR },
			"_EmissiveMap", Constants::Uniforms::EmissiveMap, Constants::Assets::DefaultBlackTex, matInstance, assetManager);

		// ORM (Occlusion, Roughness, Metallic) -> For pbr materials
		LoadTextureToUniform(matName, modelFilePath, scene, aiMat,
			{ aiTextureType_UNKNOWN, aiTextureType_METALNESS },
			"_ORMMap", Constants::Uniforms::MetallicRoughnessMap, Constants::Assets::DefaultWhiteTex, matInstance, assetManager);
	}

	void ModelImporter::LoadTextureToUniform(const std::string& matName, const std::string& modelFilePath, const aiScene* scene, const aiMaterial* aiMat, const std::vector<aiTextureType>& typesToCheck, const std::string& texNameSuffix, const std::string& uniformName, const std::string& fallbackTex, SharedPtr<MaterialInstance>& matInstance, AssetManager& assetManager)
	{
		aiTextureType validTextureType = aiTextureType_NONE;
		for (auto type : typesToCheck)
		{
			if (aiMat->GetTextureCount(type) > 0)
			{
				validTextureType = type;
				break;
			}
		}

		if (validTextureType != aiTextureType_NONE)
		{
			aiString texPath;
			aiMat->GetTexture(validTextureType, 0, &texPath);
			std::string texName = matName + texNameSuffix;

			const aiTexture* embeddedTexture = scene->GetEmbeddedTexture(texPath.C_Str());

			if (embeddedTexture)
			{
				stbi_set_flip_vertically_on_load(false);
				int width, height, channels;
				unsigned char* image_data = nullptr;

				if (embeddedTexture->mHeight == 0)
				{
					image_data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(embeddedTexture->pcData), embeddedTexture->mWidth, &width, &height, &channels, 4);
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
					std::filesystem::path outTexPath = modelPath.parent_path() / (texName + ".png");

					if (!std::filesystem::exists(outTexPath))
						stbi_write_png(outTexPath.string().c_str(), width, height, 4, image_data, width * 4);

					auto map = assetManager.Load<Texture>(texName, outTexPath.string());
					map->SetIsEngineAsset(false);
					matInstance->SetUniform(uniformName, map);

					if (embeddedTexture->mHeight == 0) stbi_image_free(image_data);
				}
				else
				{
					matInstance->SetUniform(uniformName, assetManager.GetAsset<Texture>(fallbackTex));
				}
			}
			else
			{
				std::filesystem::path modelPath(modelFilePath);
				std::filesystem::path fullTexPath = modelPath.parent_path() / texPath.C_Str();
				auto map = assetManager.Load<Texture>(texName, fullTexPath.generic_string());
				map->SetIsEngineAsset(false);
				matInstance->SetUniform(uniformName, map);
			}
		}
		else
		{
			matInstance->SetUniform(uniformName, assetManager.GetAsset<Texture>(fallbackTex));
		}
	}

}