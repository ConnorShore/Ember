#include "ModelImporter.h"

#include <Ember/Core/Core.h>
#include <Ember/Asset/MeshHeader.h>

#include <filesystem>
#include <fstream>

#include <ryml.hpp>
#include <ryml_std.hpp>

#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

namespace Ember {

	std::optional<ModelCookReport> ModelImporter::CookModel(const std::string& inputFile, const std::string& outputDirectory)
	{
		Assimp::Importer importer;

		unsigned int importFlags =
			aiProcess_Triangulate |
			aiProcess_FlipUVs |
			aiProcess_JoinIdenticalVertices |
			aiProcess_GenNormals |
			aiProcess_CalcTangentSpace;

		std::string extension = std::filesystem::path(inputFile).extension().string();
		if (extension != ".glb" && extension != ".gltf")
		{
			importFlags |= aiProcess_FlipUVs;
		}

		const aiScene* scene = importer.ReadFile(inputFile, importFlags);
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			EB_CORE_ERROR("Assimp Error: {0}", importer.GetErrorString());
			return std::nullopt;
		}

		std::string modelName = std::filesystem::path(inputFile).stem().string();
		ModelCookReport report;

		// Process Materials (Cooks to .ebmat)
		if (scene->HasMaterials())
		{
			report.Materials.reserve(scene->mNumMaterials);
			for (unsigned int i = 0; i < scene->mNumMaterials; i++)
			{
				aiMaterial* material = scene->mMaterials[i];
				CookedAssetInfo matInfo = ProcessMaterial(modelName, inputFile, scene, material, outputDirectory);
				report.Materials.push_back(matInfo);
			}
		}

		// Process Meshes Upfront (Cooks to .ebmesh)
		if (scene->HasMeshes())
		{
			report.Meshes.reserve(scene->mNumMeshes);
			for (unsigned int i = 0; i < scene->mNumMeshes; i++)
			{
				aiMesh* mesh = scene->mMeshes[i];
				CookedAssetInfo meshInfo = ProcessMesh(modelName, mesh, outputDirectory);
				report.Meshes.push_back(meshInfo);
			}
		}

		// Process Scene Graph (Builds the hierarchy mapping)
		CookedModelNode rootNode = ProcessScene(modelName, scene, outputDirectory, report.Meshes);

		// Construct the .ebmodel YAML Manifest
		std::filesystem::path manifestPath = std::filesystem::path(outputDirectory) / (modelName + ".ebmodel");
		UUID modelUUID = UUID(); // Generate an identity for the Model itself

		ryml::Tree tree;
		ryml::NodeRef root = tree.rootref();
		root |= ryml::MAP;

		root["Model"] << modelName;
		root["UUID"] << (uint64_t)modelUUID; // Bake the UUID so it's consistent!

		auto serializeNode = [](ryml::NodeRef& yamlNode, const CookedModelNode& modelNode, const std::vector<CookedAssetInfo>& mats, auto& serializeRef) -> void
			{
				yamlNode |= ryml::MAP;
				yamlNode["Name"] << modelNode.Name;

				ryml::NodeRef transformNode = yamlNode["Transform"];
				transformNode |= ryml::SEQ;
				const float* matPtr = (const float*)&modelNode.LocalTransform;
				for (int i = 0; i < 16; ++i)
				{
					transformNode.append_child() << matPtr[i];
				}

				if (!modelNode.Meshes.empty())
				{
					ryml::NodeRef meshesRef = yamlNode["Meshes"];
					meshesRef |= ryml::SEQ;
					for (const auto& mesh : modelNode.Meshes)
					{
						ryml::NodeRef m = meshesRef.append_child();
						m |= ryml::MAP;
						m["MeshID"] << (uint64_t)mesh.MeshID;

						// Resolve Assimp index to our Cooked UUID
						UUID actualMaterialID = mats[mesh.MaterialIndex].id;
						m["MaterialID"] << (uint64_t)actualMaterialID;
					}
				}

				if (!modelNode.ChildNodes.empty())
				{
					ryml::NodeRef childrenRef = yamlNode["Children"];
					childrenRef |= ryml::SEQ;
					for (const auto& child : modelNode.ChildNodes)
					{
						ryml::NodeRef c = childrenRef.append_child();
						serializeRef(c, child, mats, serializeRef);
					}
				}
			};

		ryml::NodeRef rootNodeRef = root["RootNode"];
		serializeNode(rootNodeRef, rootNode, report.Materials, serializeNode);

		std::string yamlOutput = ryml::emitrs_yaml<std::string>(tree);
		std::ofstream fout(manifestPath);
		fout << yamlOutput;
		fout.close();

		// 5. Finalize the report
		report.Model = { modelUUID, modelName, manifestPath.string() };
		return report;
	}

	CookedModelNode ModelImporter::ProcessScene(const std::string& name, const aiScene* scene, const std::string& outputDirectory, const std::vector<CookedAssetInfo>& cookedMeshes)
	{
		CookedModelNode rootNode;
		rootNode.Name = scene->mRootNode->mName.C_Str();
		rootNode.LocalTransform = ConvertMatrix(scene->mRootNode->mTransformation);
		ProcessNode(name, scene->mRootNode, rootNode, scene, outputDirectory, cookedMeshes);
		return rootNode;
	}

	Matrix4f ModelImporter::ConvertMatrix(const aiMatrix4x4& aiMat)
	{
		return Matrix4f(
			aiMat.a1, aiMat.b1, aiMat.c1, aiMat.d1,
			aiMat.a2, aiMat.b2, aiMat.c2, aiMat.d2,
			aiMat.a3, aiMat.b3, aiMat.c3, aiMat.d3,
			aiMat.a4, aiMat.b4, aiMat.c4, aiMat.d4
		);
	}

	void ModelImporter::ProcessNode(const std::string& name, aiNode* aiNode, CookedModelNode& modelNode, const aiScene* scene, const std::string& outputDirectory, const std::vector<CookedAssetInfo>& cookedMeshes)
	{
		for (unsigned int i = 0; i < aiNode->mNumMeshes; i++)
		{
			unsigned int meshIndex = aiNode->mMeshes[i];
			aiMesh* mesh = scene->mMeshes[meshIndex];

			CookedMeshNode meshNode;
			meshNode.MaterialIndex = mesh->mMaterialIndex;
			// Retrieve the UUID from the meshes we cooked upfront
			meshNode.MeshID = cookedMeshes[meshIndex].id;

			modelNode.Meshes.push_back(meshNode);
		}

		modelNode.ChildNodes.reserve(aiNode->mNumChildren);
		for (unsigned int i = 0; i < aiNode->mNumChildren; i++)
		{
			CookedModelNode childNode;
			childNode.Name = aiNode->mChildren[i]->mName.C_Str();
			childNode.LocalTransform = ConvertMatrix(aiNode->mChildren[i]->mTransformation);
			ProcessNode(name, aiNode->mChildren[i], childNode, scene, outputDirectory, cookedMeshes);
			modelNode.ChildNodes.push_back(childNode);
		}
	}

	bool ModelImporter::CookMesh(const std::vector<MeshVertex>& vertices, const std::vector<uint32_t>& indices, const std::string& outputFilePath)
	{
		std::ofstream file(outputFilePath, std::ios::binary | std::ios::trunc);
		if (!file.is_open()) return false;

		MeshHeader header;
		header.VertexCount = static_cast<uint32_t>(vertices.size());
		header.IndexCount = static_cast<uint32_t>(indices.size());

		Vector3f minBounds(FLT_MAX);
		Vector3f maxBounds(-FLT_MAX);
		for (const auto& v : vertices)
		{
			minBounds = Math::Min(minBounds, v.Position);
			maxBounds = Math::Max(maxBounds, v.Position);
		}
		header.Bounds.Min[0] = minBounds.x; header.Bounds.Min[1] = minBounds.y; header.Bounds.Min[2] = minBounds.z;
		header.Bounds.Max[0] = maxBounds.x; header.Bounds.Max[1] = maxBounds.y; header.Bounds.Max[2] = maxBounds.z;

		file.write((const char*)&header, sizeof(MeshHeader));
		file.write((const char*)vertices.data(), vertices.size() * sizeof(MeshVertex));
		file.write((const char*)indices.data(), indices.size() * sizeof(uint32_t));

		file.close();
		return true;
	}

	CookedAssetInfo ModelImporter::ProcessMesh(const std::string& name, const aiMesh* aiMesh, const std::string& outputDirectory)
	{
		std::vector<MeshVertex> vertices;
		std::vector<unsigned int> indices;

		vertices.reserve(aiMesh->mNumVertices);

		for (unsigned int i = 0; i < aiMesh->mNumVertices; i++)
		{
			MeshVertex vertex;
			vertex.Position = { aiMesh->mVertices[i].x, aiMesh->mVertices[i].y, aiMesh->mVertices[i].z };
			vertex.Normal = aiMesh->mNormals ? Vector3f(aiMesh->mNormals[i].x, aiMesh->mNormals[i].y, aiMesh->mNormals[i].z) : Vector3f(0.0f);
			vertex.TexCoords = aiMesh->mTextureCoords[0] ? Vector2f(aiMesh->mTextureCoords[0][i].x, aiMesh->mTextureCoords[0][i].y) : Vector2f(0.0f);
			vertex.Tangent = aiMesh->mTangents ? Vector3f(aiMesh->mTangents[i].x, aiMesh->mTangents[i].y, aiMesh->mTangents[i].z) : Vector3f(1.0f, 0.0f, 0.0f);
			vertex.Bitangent = aiMesh->mBitangents ? Vector3f(aiMesh->mBitangents[i].x, aiMesh->mBitangents[i].y, aiMesh->mBitangents[i].z) : Vector3f(0.0f, 1.0f, 0.0f);
			vertices.push_back(vertex);
		}

		for (unsigned int i = 0; i < aiMesh->mNumFaces; i++)
		{
			for (unsigned int j = 0; j < aiMesh->mFaces[i].mNumIndices; j++)
			{
				indices.push_back(aiMesh->mFaces[i].mIndices[j]);
			}
		}

		UUID meshUUID = UUID();
		std::string meshFileName = name + "_" + aiMesh->mName.C_Str() + ".ebmesh";
		std::filesystem::path outputPath = std::filesystem::path(outputDirectory) / meshFileName;

		if (CookMesh(vertices, indices, outputPath.string()))
		{
			return { meshUUID, meshFileName, outputPath.string() };
		}

		EB_CORE_ERROR("Failed to cook mesh: {0}", meshFileName);
		return { 0, "", "" };
	}

	// --- MATERIAL COOKING ---

	CookedAssetInfo ModelImporter::ProcessMaterial(const std::string& modelName, const std::string& modelFilePath, const aiScene* scene, const aiMaterial* aiMat, const std::string& outputDirectory)
	{
		std::string rawMatName = aiMat->GetName().C_Str();
		std::string matName = modelName + "_" + rawMatName;

		CookedMaterialDef def;
		def.BaseMaterial = DetermineBaseMaterial(aiMat);

		ExtractPBRUniforms(aiMat, def);
		ExtractTextures(matName, modelFilePath, scene, aiMat, def, outputDirectory);

		UUID materialUUID = UUID();
		std::string matFileName = matName + ".ebmat";
		std::filesystem::path manifestPath = std::filesystem::path(outputDirectory) / matFileName;

		ryml::Tree tree;
		ryml::NodeRef root = tree.rootref();
		root |= ryml::MAP;

		root["Material"] << matName;
		root["UUID"] << (uint64_t)materialUUID;
		root["BaseMaterial"] << def.BaseMaterial;

		ryml::NodeRef uniformsNode = root["Uniforms"];
		uniformsNode |= ryml::MAP;

		ryml::NodeRef albedoNode = uniformsNode["Albedo"];
		albedoNode |= ryml::SEQ;
		albedoNode.append_child() << def.AlbedoColor.x;
		albedoNode.append_child() << def.AlbedoColor.y;
		albedoNode.append_child() << def.AlbedoColor.z;

		uniformsNode["Emission"] << def.Emission;
		uniformsNode["Roughness"] << def.Roughness;
		uniformsNode["Metallic"] << def.Metallic;

		ryml::NodeRef texturesNode = root["Textures"];
		texturesNode |= ryml::MAP;
		if (!def.AlbedoMapPath.empty()) texturesNode["AlbedoMap"] << def.AlbedoMapPath;
		if (!def.NormalMapPath.empty()) texturesNode["NormalMap"] << def.NormalMapPath;
		if (!def.EmissiveMapPath.empty()) texturesNode["EmissiveMap"] << def.EmissiveMapPath;
		if (!def.ORMMapPath.empty()) texturesNode["ORMMap"] << def.ORMMapPath;

		std::string yamlOutput = ryml::emitrs_yaml<std::string>(tree);
		std::ofstream fout(manifestPath);
		fout << yamlOutput;
		fout.close();

		return { materialUUID, matName, manifestPath.string() };
	}

	std::string ModelImporter::DetermineBaseMaterial(const aiMaterial* aiMat)
	{
		std::string rawMatName = aiMat->GetName().C_Str();
		std::string baseMatName = Constants::Assets::StandardGeometryMat;

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

	void ModelImporter::ExtractPBRUniforms(const aiMaterial* aiMat, CookedMaterialDef& def)
	{
		aiColor4D diffuseColor(1.0f, 1.0f, 1.0f, 1.0f);
		if (AI_SUCCESS != aiGetMaterialColor(aiMat, AI_MATKEY_BASE_COLOR, &diffuseColor))
		{
			aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_DIFFUSE, &diffuseColor);
		}
		def.AlbedoColor = { diffuseColor.r, diffuseColor.g, diffuseColor.b };

		aiColor4D emissiveColor(0.0f, 0.0f, 0.0f, 1.0f);
		aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_EMISSIVE, &emissiveColor);

		float maxEmission = std::max({ emissiveColor.r, emissiveColor.g, emissiveColor.b });
		if (def.BaseMaterial == Constants::Assets::StandardUnlitMat && maxEmission > 0.0f)
		{
			if (maxEmission > 1.0f)
			{
				def.Emission = maxEmission;
				def.AlbedoColor = { emissiveColor.r / maxEmission, emissiveColor.g / maxEmission, emissiveColor.b / maxEmission };
			}
			else
			{
				def.Emission = 1.0f;
				def.AlbedoColor = { emissiveColor.r, emissiveColor.g, emissiveColor.b };
			}
		}
		else
		{
			def.Emission = maxEmission;
			if (def.Emission == 0.0f && aiMat->GetTextureCount(aiTextureType_EMISSIVE) > 0)
			{
				def.Emission = 1.0f;
			}
		}

		aiGetMaterialFloat(aiMat, AI_MATKEY_ROUGHNESS_FACTOR, &def.Roughness);
		aiGetMaterialFloat(aiMat, AI_MATKEY_METALLIC_FACTOR, &def.Metallic);
	}

	void ModelImporter::ExtractTextures(const std::string& matName, const std::string& modelFilePath, const aiScene* scene, const aiMaterial* aiMat, CookedMaterialDef& def, const std::string& outputDirectory)
	{
		def.AlbedoMapPath = ExtractAndCopyTexture(matName, modelFilePath, scene, aiMat, { aiTextureType_BASE_COLOR, aiTextureType_DIFFUSE }, "_AlbedoMap", outputDirectory);
		def.NormalMapPath = ExtractAndCopyTexture(matName, modelFilePath, scene, aiMat, { aiTextureType_NORMALS, aiTextureType_HEIGHT }, "_NormalMap", outputDirectory);
		def.EmissiveMapPath = ExtractAndCopyTexture(matName, modelFilePath, scene, aiMat, { aiTextureType_EMISSIVE, aiTextureType_EMISSION_COLOR }, "_EmissiveMap", outputDirectory);
		def.ORMMapPath = ExtractAndCopyTexture(matName, modelFilePath, scene, aiMat, { aiTextureType_UNKNOWN, aiTextureType_METALNESS }, "_ORMMap", outputDirectory);
	}

	std::string ModelImporter::ExtractAndCopyTexture(const std::string& matName, const std::string& modelFilePath, const aiScene* scene, const aiMaterial* aiMat, const std::vector<aiTextureType>& typesToCheck, const std::string& texNameSuffix, const std::string& outputDirectory)
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

		if (validTextureType == aiTextureType_NONE) return "";

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
				std::string finalFileName = texName + ".png";
				std::filesystem::path outTexPath = std::filesystem::path(outputDirectory) / finalFileName;

				if (!std::filesystem::exists(outTexPath))
					stbi_write_png(outTexPath.string().c_str(), width, height, 4, image_data, width * 4);

				if (embeddedTexture->mHeight == 0) stbi_image_free(image_data);

				return finalFileName;
			}
		}
		else
		{
			std::filesystem::path modelPath(modelFilePath);
			std::filesystem::path sourceTexPath = modelPath.parent_path() / texPath.C_Str();

			if (std::filesystem::exists(sourceTexPath))
			{
				std::string finalFileName = sourceTexPath.filename().string();
				std::filesystem::path destTexPath = std::filesystem::path(outputDirectory) / finalFileName;

				if (!std::filesystem::exists(destTexPath))
					std::filesystem::copy_file(sourceTexPath, destTexPath);

				return finalFileName;
			}
		}

		return "";
	}
}