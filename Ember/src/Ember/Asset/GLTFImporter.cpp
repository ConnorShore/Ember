#include "ebpch.h"
#include "GLTFImporter.h"
#include "Ember/Core/Core.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <tiny_gltf.h>

namespace Ember {

	SharedPtr<Model> GLTFImporter::LoadModel(const std::string& filePath)
	{
		// TODO: Implement this! For now, we'll just return nullptr to avoid linker errors since the function is declared in the header.
		// but we will want to load the entire model hierarchy, including all meshes, materials, textures, skeletons, animations, etc.
		// For now, we'll just load the skeleton and a single skinned mesh as a proof of concept.
		return nullptr;
	}

	SharedPtr<Skeleton> GLTFImporter::LoadSkeleton(const std::string& filepath)
	{
		tinygltf::Model model;
		tinygltf::TinyGLTF loader;
		std::string err, warn;

		bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, filepath);
		if (!ret) {
			EB_CORE_ERROR("Failed to load GLB: {}", err);
			return nullptr;
		}

		// 1. Grab the Skin (This is the Rosetta Stone for animations)
		if (model.skins.empty()) {
			EB_CORE_ERROR("Model does not have a skin!");
			return nullptr;
		}
		const tinygltf::Skin& skin = model.skins[0];

		std::vector<Bone> bones(skin.joints.size());
		std::vector<Matrix4f> inverseBindMatrices(skin.joints.size());

		// Extract the mathematically perfect Inverse Bind Matrices from the file
		if (skin.inverseBindMatrices > -1)
		{
			const tinygltf::Accessor& accessor = model.accessors[skin.inverseBindMatrices];
			const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

			const float* matrixData = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
			for (size_t i = 0; i < skin.joints.size(); i++) {
				// glTF stores matrices as column-major float arrays, exactly like GLM
				inverseBindMatrices[i] = Math::MakeMatrix4f(matrixData + (i * 16));
			}
		}

		// Map only the valid joints from the Skin, not all nodes
		for (size_t i = 0; i < skin.joints.size(); i++)
		{
			int nodeIndex = skin.joints[i];
			const auto& node = model.nodes[nodeIndex];

			bones[i].Name = node.name.empty() ? "Bone_" + std::to_string(i) : node.name;
			bones[i].ParentID = -1; // Default to no parent

			// Extract Local Pose
			BoneTransform localPose;
			if (node.translation.size() == 3) {
				localPose.Translation = Vector3f(node.translation[0], node.translation[1], node.translation[2]);
			}
			else {
				localPose.Translation = Vector3f(0.0f);
			}
			if (node.rotation.size() == 4) {
				localPose.Rotation = Quaternion(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
			}
			else {
				localPose.Rotation = Quaternion(1.0f, 0.0f, 0.0f, 0.0f);
			}
			bones[i].LocalBindPoseTransform = localPose;
		}

		// Map the parent/child hierarchy specifically within our new Bones array
		for (size_t i = 0; i < skin.joints.size(); i++)
		{
			int nodeIndex = skin.joints[i];
			const auto& node = model.nodes[nodeIndex];

			for (int childNodeIndex : node.children)
			{
				// See if this child node is actually a bone in our skin
				auto it = std::find(skin.joints.begin(), skin.joints.end(), childNodeIndex);
				if (it != skin.joints.end()) {
					// It is a bone, assign i as its parent.
					int childBoneIndex = std::distance(skin.joints.begin(), it);
					bones[childBoneIndex].ParentID = static_cast<uint32_t>(i);
				}
			}
		}

		std::string name = std::filesystem::path(filepath).stem().string() + "_Skeleton";
		return SharedPtr<Skeleton>::Create(name, bones, inverseBindMatrices);
	}

	SharedPtr<SkinnedMesh> GLTFImporter::LoadSkinnedMesh(const std::string& filePath)
	{
		tinygltf::Model model;
		tinygltf::TinyGLTF loader;
		std::string err, warn;

		bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, filePath);
		if (!ret) {
			EB_CORE_ERROR("Failed to load GLB: {}", err);
			return nullptr;
		}

		// Grab the first primitive of the first mesh
		const tinygltf::Primitive& primitive = model.meshes[0].primitives[0];

		// Find the position accessor to determine vertex count
		const tinygltf::Accessor& posAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
		std::vector<SkinnedMeshVertex> vertices(posAccessor.count);

		// Safely initialize all vertex memory to prevent NaN shader crashes!
		for (auto& v : vertices) {
			v.Position = Vector3f(0.0f);
			v.Normal = Vector3f(0.0f, 1.0f, 0.0f);
			v.TexCoords = Vector2f(0.0f);
			v.Tangent = Vector3f(1.0f, 0.0f, 0.0f);   // Default Tangent
			v.Bitangent = Vector3f(0.0f, 0.0f, 1.0f); // Default Bitangent
			v.BoneIDs[0] = 0; v.BoneIDs[1] = 0; v.BoneIDs[2] = 0; v.BoneIDs[3] = 0;
			v.BoneWeights[0] = 0.0f; v.BoneWeights[1] = 0.0f; v.BoneWeights[2] = 0.0f; v.BoneWeights[3] = 0.0f;
		}

		// =========================================================
		// HELPER LAMBDA: Extracts any float-based VEC2/VEC3 attribute
		// =========================================================
		auto extractFloatAttribute = [&](const std::string& attrName, int numComponents, auto assignmentLambda)
			{
				if (primitive.attributes.find(attrName) != primitive.attributes.end())
				{
					const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.find(attrName)->second];
					const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
					const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

					int stride = accessor.ByteStride(bufferView);
					const unsigned char* dataPtr = &buffer.data[bufferView.byteOffset + accessor.byteOffset];

					for (size_t i = 0; i < accessor.count; i++) {
						const float* vec = reinterpret_cast<const float*>(dataPtr + (i * stride));
						assignmentLambda(vertices[i], vec);
					}
				}
			};

		// 1. EXTRACT STANDARD GEOMETRY
		extractFloatAttribute("POSITION", 3, [](SkinnedMeshVertex& v, const float* data) {
			v.Position = Vector3f(data[0], data[1], data[2]);
			});
		extractFloatAttribute("NORMAL", 3, [](SkinnedMeshVertex& v, const float* data) {
			v.Normal = Vector3f(data[0], data[1], data[2]);
			});
		extractFloatAttribute("TEXCOORD_0", 2, [](SkinnedMeshVertex& v, const float* data) {
			v.TexCoords = Vector2f(data[0], data[1]);
			});

		// 2. EXTRACT BONE WEIGHTS (Float4)
		extractFloatAttribute("WEIGHTS_0", 4, [](SkinnedMeshVertex& v, const float* data) {
			v.BoneWeights[0] = data[0]; v.BoneWeights[1] = data[1];
			v.BoneWeights[2] = data[2]; v.BoneWeights[3] = data[3];
			});

		// 3. EXTRACT BONE IDs (Requires safe casting from uint8/uint16)
		if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end())
		{
			const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.find("JOINTS_0")->second];
			const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
			int stride = accessor.ByteStride(bufferView);
			const unsigned char* dataPtr = &buffer.data[bufferView.byteOffset + accessor.byteOffset];

			for (size_t i = 0; i < accessor.count; i++)
			{
				if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
					const uint16_t* joints = reinterpret_cast<const uint16_t*>(dataPtr + (i * stride));
					vertices[i].BoneIDs[0] = joints[0]; vertices[i].BoneIDs[1] = joints[1];
					vertices[i].BoneIDs[2] = joints[2]; vertices[i].BoneIDs[3] = joints[3];
				}
				else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
					const uint8_t* joints = reinterpret_cast<const uint8_t*>(dataPtr + (i * stride));
					vertices[i].BoneIDs[0] = joints[0]; vertices[i].BoneIDs[1] = joints[1];
					vertices[i].BoneIDs[2] = joints[2]; vertices[i].BoneIDs[3] = joints[3];
				}
			}
		}

		// 4. EXTRACT INDICES
		std::vector<uint32_t> indices;
		if (primitive.indices >= 0)
		{
			const tinygltf::Accessor& accessor = model.accessors[primitive.indices];
			const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

			indices.resize(accessor.count);
			int stride = accessor.ByteStride(bufferView);
			const unsigned char* dataPtr = &buffer.data[bufferView.byteOffset + accessor.byteOffset];

			for (size_t i = 0; i < accessor.count; i++)
			{
				if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
					indices[i] = *reinterpret_cast<const uint32_t*>(dataPtr + (i * stride));
				}
				else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
					indices[i] = *reinterpret_cast<const uint16_t*>(dataPtr + (i * stride));
				}
			}
		}

		std::string name = std::filesystem::path(filePath).stem().string() + "_Mesh";
		return SharedPtr<SkinnedMesh>::Create(UUID(), name, vertices, indices);
	}
}