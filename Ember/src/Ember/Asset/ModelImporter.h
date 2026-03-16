#pragma once

#include "Model.h"
#include "Ember/Render/Mesh.h"
#include "Ember/Core/Core.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <string>

namespace Ember {


	class ModelImporter
	{
	public:
		static SharedPtr<Model> Load(const std::string& name, const std::string& filePath)
		{
			Assimp::Importer importer;
			const aiScene* scene = importer.ReadFile(filePath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);
			m_MeshCounter = 0;

			auto rootModelNode = ProcessScene(name, scene);
			return SharedPtr<Model>::Create(name, filePath, rootModelNode);
		}

	private:
		inline static unsigned int m_MeshCounter = 0;

		static ModelNode ProcessScene(const std::string& name, const aiScene* scene)
		{
			ModelNode rootNode;
			rootNode.Name = scene->mRootNode->mName.C_Str();
			rootNode.LocalTransform = ConvertMatrix(scene->mRootNode->mTransformation);
			ProcessNode(name, scene->mRootNode, rootNode, scene);
			return rootNode;
		}

		static Matrix4f ConvertMatrix(const aiMatrix4x4& aiMat)
		{
			return Matrix4f(
				aiMat.a1, aiMat.a2, aiMat.a3, aiMat.a4,
				aiMat.b1, aiMat.b2, aiMat.b3, aiMat.b4,
				aiMat.c1, aiMat.c2, aiMat.c3, aiMat.c4,
				aiMat.d1, aiMat.d2, aiMat.d3, aiMat.d4
			);
		}

		static void ProcessNode(const std::string& name, aiNode* aiNode, ModelNode& modelNode, const aiScene* scene)
		{
			for (unsigned int i = 0; i < aiNode->mNumMeshes; i++)
			{
				aiMesh* mesh = scene->mMeshes[aiNode->mMeshes[i]];
				modelNode.Meshes.push_back(ProcessMesh(name, mesh));
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

		static SharedPtr<Mesh> ProcessMesh(const std::string& name, const aiMesh* aiMesh)
		{
			std::vector<float> vertices;
			std::vector<unsigned int> indices;
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
					indices.push_back(aiMesh->mFaces[i].mIndices[j]);
			}

			std::string meshName = name + "_Mesh_" + std::to_string(m_MeshCounter++);
			return SharedPtr<Mesh>::Create(meshName, vertices, indices);
		}
	};

}