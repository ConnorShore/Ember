#include "GLTFImporter.h"

#include <Ember/Core/Application.h>
#include <Ember/Core/Core.h>
#include <Ember/Asset/AssetManager.h>
#include <Ember/Asset/MeshSerializer.h>
#include <Ember/Asset/MaterialSerializer.h>
#include <Ember/Asset/SkeletonSerializer.h>
#include <Ember/Asset/AnimationSerializer.h>

#include <filesystem>
#include <fstream>
#include <unordered_map>

#include <ryml.hpp>
#include <ryml_std.hpp>

#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>

namespace Ember {

	// --- INTERNAL HELPERS ---

	static std::string SanitizeFileName(const std::string& input) {
		std::string output = input;
		const std::string invalidChars = "<>:/\\|?*\" \t\n\r";
		for (char& c : output) {
			if (invalidChars.find(c) != std::string::npos) c = '_';
		}
		return output.empty() ? "Unnamed" : output;
	}

	static Matrix4f ExtractTransform(const tinygltf::Node& node) {
		if (node.matrix.size() == 16) {
			return Matrix4f(
				(float)node.matrix[0], (float)node.matrix[1], (float)node.matrix[2], (float)node.matrix[3],
				(float)node.matrix[4], (float)node.matrix[5], (float)node.matrix[6], (float)node.matrix[7],
				(float)node.matrix[8], (float)node.matrix[9], (float)node.matrix[10], (float)node.matrix[11],
				(float)node.matrix[12], (float)node.matrix[13], (float)node.matrix[14], (float)node.matrix[15]
			);
		}
		Vector3f t(0.0f);
		Quaternion r(1.0f, 0.0f, 0.0f, 0.0f);
		Vector3f s(1.0f);
		if (node.translation.size() == 3) t = Vector3f((float)node.translation[0], (float)node.translation[1], (float)node.translation[2]);
		if (node.rotation.size() == 4) r = Quaternion((float)node.rotation[3], (float)node.rotation[0], (float)node.rotation[1], (float)node.rotation[2]);
		if (node.scale.size() == 3) s = Vector3f((float)node.scale[0], (float)node.scale[1], (float)node.scale[2]);
		return Math::Translate(t) * Math::ToMatrix4f(r) * Math::Scale(s);
	}

	// --- MAIN COOKING PIPELINE ---

	std::optional<ModelCookReport> GLTFImporter::CookModel(const std::string& inputFile, const std::string& outputDirectory) {
		tinygltf::Model model;
		tinygltf::TinyGLTF loader;
		std::string err, warn;

		if (!loader.LoadBinaryFromFile(&model, &err, &warn, inputFile)) {
			EB_CORE_ERROR("GLTF Error: {0}", err);
			return std::nullopt;
		}

		std::string modelName = SanitizeFileName(std::filesystem::path(inputFile).stem().string());
		ModelCookReport report;
		auto& am = Application::Instance().GetAssetManager();

		// 1. COOK TEXTURES
		std::vector<CookedAssetInfo> cookedImages(model.images.size());
		for (size_t i = 0; i < model.images.size(); i++) {
			const auto& img = model.images[i];
			std::string name = img.name.empty() ? "Tex_" + std::to_string(i) : SanitizeFileName(img.name);
			std::string fileName = modelName + "_" + name + ".png";
			std::string fullPath = (std::filesystem::path(outputDirectory) / fileName).string();

			if (!std::filesystem::exists(fullPath)) {
				stbi_write_png(fullPath.c_str(), img.width, img.height, img.component, img.image.data(), img.width * img.component);
			}
			cookedImages[i] = { UUID(), fileName, fullPath };
			report.Textures.push_back(cookedImages[i]);
			am.Load<Texture2D>(cookedImages[i].id, cookedImages[i].name, cookedImages[i].path, false);
		}

		// 2. COOK MATERIALS
		for (size_t i = 0; i < model.materials.size(); i++) {
			report.Materials.push_back(ProcessMaterial(modelName, (int)i, model, outputDirectory, cookedImages));
			am.Load<MaterialBase>(report.Materials.back().id, report.Materials.back().name, report.Materials.back().path, false);
		}

		// 3. COOK MESHES
		std::vector<std::vector<CookedAssetInfo>> meshToPrims(model.meshes.size());
		for (size_t i = 0; i < model.meshes.size(); i++) {
			meshToPrims[i] = ProcessMesh(modelName, (int)i, model, outputDirectory);
			for (auto& p : meshToPrims[i]) {
				report.Meshes.push_back(p);
				am.Load<Mesh>(p.id, p.name, p.path, false);
			}
		}

		// 4. COOK SKELETON
		UUID skeletonID = Constants::InvalidUUID;
		std::unordered_map<int, int> nodeToBoneMap; // Needed for animations later

		if (!model.skins.empty()) {
			const tinygltf::Skin& skin = model.skins[0];
			std::vector<Bone> bones(skin.joints.size());
			std::vector<Matrix4f> invBinds(skin.joints.size());

			// Load Inverse Bind Matrices
			if (skin.inverseBindMatrices > -1) {
				const auto& accessor = model.accessors[skin.inverseBindMatrices];
				const auto& view = model.bufferViews[accessor.bufferView];
				const auto& buffer = model.buffers[view.buffer];
				const float* matData = reinterpret_cast<const float*>(&buffer.data[view.byteOffset + accessor.byteOffset]);
				for (size_t i = 0; i < skin.joints.size(); i++) {
					invBinds[i] = glm::make_mat4(matData + (i * 16));
				}
			}

			// Map Joints
			for (size_t i = 0; i < skin.joints.size(); i++) {
				int nodeIdx = skin.joints[i];
				nodeToBoneMap[nodeIdx] = (int)i;
				const auto& gNode = model.nodes[nodeIdx];

				bones[i].Name = gNode.name.empty() ? "Bone_" + std::to_string(i) : gNode.name;
				bones[i].ParentID = -1;

				// Local Pose
				if (gNode.translation.size() == 3) bones[i].LocalBindPoseTransform.Translation = Vector3f((float)gNode.translation[0], (float)gNode.translation[1], (float)gNode.translation[2]);
				if (gNode.rotation.size() == 4) bones[i].LocalBindPoseTransform.Rotation = Quaternion((float)gNode.rotation[3], (float)gNode.rotation[0], (float)gNode.rotation[1], (float)gNode.rotation[2]);
			}

			// Hierarchy
			for (size_t i = 0; i < skin.joints.size(); i++) {
				const auto& gNode = model.nodes[skin.joints[i]];
				for (int childNodeIdx : gNode.children) {
					if (nodeToBoneMap.count(childNodeIdx)) {
						bones[nodeToBoneMap[childNodeIdx]].ParentID = (uint32_t)i;
					}
				}
			}

			skeletonID = UUID();
			auto skeleton = SharedPtr<Skeleton>::Create(skeletonID, modelName + "_Skeleton", bones, invBinds);
			std::string skelPath = (std::filesystem::path(outputDirectory) / (modelName + ".ebskeleton")).string();
			SkeletonSerializer::Serialize(skelPath, skeleton);
			report.Skeleton = { skeletonID, modelName + "_Skeleton", skelPath };
		}

		// 4.5 COOK ANIMATIONS
		for (size_t i = 0; i < model.animations.size(); i++) {
			const auto& gAnim = model.animations[i];
			std::unordered_map<uint32_t, BoneAnimationTrack> tracks;
			float duration = 0.0f;

			for (const auto& channel : gAnim.channels) {
				if (!nodeToBoneMap.count(channel.target_node)) continue;

				uint32_t boneID = nodeToBoneMap[channel.target_node];
				const auto& sampler = gAnim.samplers[channel.sampler];

				// Input (Times)
				const auto& inAcc = model.accessors[sampler.input];
				const auto& inView = model.bufferViews[inAcc.bufferView];
				const float* times = reinterpret_cast<const float*>(&model.buffers[inView.buffer].data[inView.byteOffset + inAcc.byteOffset]);

				// Output (Values)
				const auto& outAcc = model.accessors[sampler.output];
				const auto& outView = model.bufferViews[outAcc.bufferView];
				const float* vals = reinterpret_cast<const float*>(&model.buffers[outView.buffer].data[outView.byteOffset + outAcc.byteOffset]);

				if (times[inAcc.count - 1] > duration) duration = times[inAcc.count - 1];

				auto& track = tracks[boneID];
				track.BoneID = boneID;

				for (size_t k = 0; k < inAcc.count; k++) {
					if (channel.target_path == "translation")
						track.PositionKeyframes.push_back({ times[k], Vector3f(vals[k * 3], vals[k * 3 + 1], vals[k * 3 + 2]) });
					else if (channel.target_path == "rotation")
						track.RotationKeyframes.push_back({ times[k], Quaternion(vals[k * 4 + 3], vals[k * 4], vals[k * 4 + 1], vals[k * 4 + 2]) });
				}
			}

			std::vector<BoneAnimationTrack> trackList;
			for (auto& pair : tracks)
				trackList.push_back(pair.second);

			auto anim = SharedPtr<Animation>::Create(gAnim.name.empty() ? "Anim_" + std::to_string(i) : gAnim.name, duration, trackList);
			std::string animPath = (std::filesystem::path(outputDirectory) / (modelName + "_" + anim->GetName() + ".ebanim")).string();
			AnimationSerializer::Serialize(animPath, anim);
			report.Animations.push_back({ UUID(), anim->GetName(), animPath });
		}

		// 5. COOK MODEL HIERARCHY
		UUID modelUUID = UUID();
		std::string manifestPath = (std::filesystem::path(outputDirectory) / (modelName + ".ebmodel")).string();

		// First, build the intermediate hierarchy (CookedModelNode) from the glTF scene
		CookedModelNode intermediateRoot;
		intermediateRoot.Name = modelName + "_Root";
		const auto& scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];
		for (int nodeIdx : scene.nodes) {
			intermediateRoot.ChildNodes.push_back(ProcessNode(nodeIdx, model, outputDirectory, meshToPrims));
		}

		// Now, convert that intermediate hierarchy into the final ModelNode structure
		// This resolves the Mesh UUIDs into actual Mesh Asset pointers via the AssetManager
		auto convertNode = [&](const CookedModelNode& cooked, auto& self) -> ModelNode {
			ModelNode node;
			node.Name = cooked.Name;
			node.LocalTransform = cooked.LocalTransform;
			for (const auto& m : cooked.Meshes) {
				// AssetManager was updated in steps 1-3, so these lookups will succeed
				node.Meshes.push_back({ am.GetAsset<Mesh>(m.MeshID), m.MaterialIndex });
			}
			for (const auto& c : cooked.ChildNodes) {
				node.ChildNodes.push_back(self(c, self));
			}
			return node;
			};

		ModelNode rootModelNode = convertNode(intermediateRoot, convertNode);

		// Collect the actual material assets for the Model constructor
		std::vector<SharedPtr<MaterialBase>> modelMaterials;
		for (auto& matInfo : report.Materials) {
			modelMaterials.push_back(am.GetAsset<MaterialBase>(matInfo.id));
		}

		// Create the final Model Asset
		auto modelAsset = SharedPtr<Model>::Create(modelUUID, modelName, manifestPath, rootModelNode, modelMaterials, skeletonID);

		// 6. USE THE SERIALIZER!
		ModelSerializer::Serialize(manifestPath, modelAsset);

		report.Model = { modelUUID, modelName, manifestPath };
		return report;
	}

	CookedAssetInfo GLTFImporter::ProcessMaterial(const std::string& modelName, int matIndex, const tinygltf::Model& model, const std::string& outputDirectory, std::vector<CookedAssetInfo>& cookedImages) {
		const auto& gMat = model.materials[matIndex];
		std::string name = modelName + "_" + (gMat.name.empty() ? "Mat_" + std::to_string(matIndex) : SanitizeFileName(gMat.name));
		UUID id = UUID();

		// If model is skinned, use StandardSkinnedGeometryShader
		SharedPtr<Shader> shader;
		if (!model.skins.empty())
			shader = Application::Instance().GetAssetManager().GetAsset<Shader>(Constants::Assets::StandardSkinnedGeometryShadUUID);
		else
			shader = Application::Instance().GetAssetManager().GetAsset<Shader>(Constants::Assets::StandardGeometryShadUUID);

		auto mat = SharedPtr<Material>::Create(id, name, shader, RenderQueue::Opaque);
		mat->SetIsEngineAsset(false);

		auto& pbr = gMat.pbrMetallicRoughness;
		mat->SetUniform(Constants::Uniforms::Albedo, Vector3f((float)pbr.baseColorFactor[0], (float)pbr.baseColorFactor[1], (float)pbr.baseColorFactor[2]));
		mat->SetUniform(Constants::Uniforms::Metallic, (float)pbr.metallicFactor);
		mat->SetUniform(Constants::Uniforms::Roughness, (float)pbr.roughnessFactor);

		// Set defaults for properties not in glTF
		mat->SetUniform(Constants::Uniforms::AO, 1.0f); 
		mat->SetUniform(Constants::Uniforms::EmissionColor, Vector3f(1.0f));
		mat->SetUniform(Constants::Uniforms::Emission, 0.0f);

		auto& am = Application::Instance().GetAssetManager();
		auto defaultWhite = am.GetAsset<Texture2D>(Constants::Assets::DefaultWhiteTexUUID);
		auto defaultNormal = am.GetAsset<Texture2D>(Constants::Assets::DefaultNormalTexUUID);
		auto defaultBlack = am.GetAsset<Texture2D>(Constants::Assets::DefaultBlackTexUUID);

		auto assignTex = [&](int texIdx, const std::string& uniName, SharedPtr<Texture2D> fallback) {
			if (texIdx > -1) 
			{
				int imgIdx = model.textures[texIdx].source;
				auto tex = Application::Instance().GetAssetManager().GetAsset<Texture2D>(cookedImages[imgIdx].id);
				mat->SetUniform(uniName, tex);
			}
			else 
			{
				mat->SetUniform(uniName, fallback);
			}
			};

		assignTex(pbr.baseColorTexture.index, Constants::Uniforms::AlbedoMap, defaultWhite);
		assignTex(gMat.normalTexture.index, Constants::Uniforms::NormalMap, defaultNormal);
		assignTex(pbr.metallicRoughnessTexture.index, Constants::Uniforms::MetallicRoughnessMap, defaultWhite);

		std::string path = (std::filesystem::path(outputDirectory) / (name + ".ebmat")).string();
		MaterialSerializer::Serialize(path, mat);
		return { id, name, path };
	}

	std::vector<CookedAssetInfo> GLTFImporter::ProcessMesh(const std::string& modelName, int meshIndex, const tinygltf::Model& model, const std::string& outputDirectory) {
		std::vector<CookedAssetInfo> cookedPrims;
		const tinygltf::Mesh& gltfMesh = model.meshes[meshIndex];
		std::string safeMeshName = SanitizeFileName(gltfMesh.name.empty() ? "Mesh_" + std::to_string(meshIndex) : gltfMesh.name);

		for (size_t primIndex = 0; primIndex < gltfMesh.primitives.size(); primIndex++) {
			const auto& primitive = gltfMesh.primitives[primIndex];
			auto posIt = primitive.attributes.find("POSITION");
			if (posIt == primitive.attributes.end()) continue;

			const auto& posAccessor = model.accessors[posIt->second];
			uint32_t vertexCount = (uint32_t)posAccessor.count;
			bool isSkinned = primitive.attributes.count("JOINTS_0");
			std::vector<SkinnedMeshVertex> vertices(vertexCount);

			auto extract = [&](const std::string& attr, auto func) {
				if (!primitive.attributes.count(attr)) return;
				const auto& acc = model.accessors[primitive.attributes.find(attr)->second];
				const auto& view = model.bufferViews[acc.bufferView];
				int stride = acc.ByteStride(view);
				const unsigned char* data = &model.buffers[view.buffer].data[view.byteOffset + acc.byteOffset];
				for (size_t i = 0; i < acc.count; i++) func(vertices[i], data + (i * stride), acc.componentType);
				};

			extract("POSITION", [](SkinnedMeshVertex& v, const unsigned char* d, int t) { const float* f = (float*)d; v.Position = { f[0], f[1], f[2] }; });
			extract("NORMAL", [](SkinnedMeshVertex& v, const unsigned char* d, int t) { const float* f = (float*)d; v.Normal = { f[0], f[1], f[2] }; });
			extract("TEXCOORD_0", [](SkinnedMeshVertex& v, const unsigned char* d, int t) { const float* f = (float*)d; v.TexCoords = { f[0], f[1] }; });
			extract("WEIGHTS_0", [](SkinnedMeshVertex& v, const unsigned char* d, int t) { const float* f = (float*)d; v.BoneWeights[0] = f[0]; v.BoneWeights[1] = f[1]; v.BoneWeights[2] = f[2]; v.BoneWeights[3] = f[3]; });
			extract("JOINTS_0", [](SkinnedMeshVertex& v, const unsigned char* d, int t) {
				if (t == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) { const uint16_t* s = (uint16_t*)d; v.BoneIDs[0] = s[0]; v.BoneIDs[1] = s[1]; v.BoneIDs[2] = s[2]; v.BoneIDs[3] = s[3]; }
				else { v.BoneIDs[0] = d[0]; v.BoneIDs[1] = d[1]; v.BoneIDs[2] = d[2]; v.BoneIDs[3] = d[3]; }
				});

			std::vector<uint32_t> indices;
			if (primitive.indices >= 0) {
				const auto& acc = model.accessors[primitive.indices];
				const auto& view = model.bufferViews[acc.bufferView];
				indices.resize(acc.count);
				const unsigned char* data = &model.buffers[view.buffer].data[view.byteOffset + acc.byteOffset];
				int stride = acc.ByteStride(view);
				for (size_t i = 0; i < acc.count; i++) {
					if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) indices[i] = *(uint32_t*)(data + i * stride);
					else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) indices[i] = *(uint16_t*)(data + i * stride);
					else indices[i] = *(data + i * stride);
				}
			}
			else
			{
				// No indices provided, generate a simple 0..vertexCount-1 index buffer
				indices.resize(vertices.size());
				for (uint32_t i = 0; i < vertices.size(); i++) {
					indices[i] = i;
				}
			}

			UUID meshUUID = UUID();
			std::string primFileName = modelName + "_" + safeMeshName + "_Prim" + std::to_string(primIndex) + ".ebmesh";
			std::string outputPath = (std::filesystem::path(outputDirectory) / primFileName).string();

			// Cook directly to disk! No Mesh object created yet.
			if (MeshSerializer::Serialize(outputPath, vertices, indices, isSkinned))
				cookedPrims.push_back({ meshUUID, primFileName, outputPath });
		}

		return cookedPrims;
	}

	CookedModelNode GLTFImporter::ProcessNode(int nodeIndex, const tinygltf::Model& model, const std::string& outputDirectory, const std::vector<std::vector<CookedAssetInfo>>& meshToPrims) {
		const auto& gNode = model.nodes[nodeIndex];
		CookedModelNode node;
		node.Name = gNode.name.empty() ? "Node_" + std::to_string(nodeIndex) : gNode.name;
		node.LocalTransform = ExtractTransform(gNode);
		if (gNode.mesh > -1) {
			const auto& cooked = meshToPrims[gNode.mesh];
			for (size_t i = 0; i < model.meshes[gNode.mesh].primitives.size(); i++)
				node.Meshes.push_back({ cooked[i].id, (uint32_t)model.meshes[gNode.mesh].primitives[i].material });
		}
		for (int childIdx : gNode.children)
			node.ChildNodes.push_back(ProcessNode(childIdx, model, outputDirectory, meshToPrims));
		return node;
	}
}