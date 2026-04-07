#pragma once

#include "Model.h"
#include "Skeleton.h"
#include "Ember/Render/SkinnedMesh.h"

#include <string>

namespace Ember {

	class GLTFImporter
	{
	public:
		static SharedPtr<Model> LoadModel(const std::string& filePath);
		static SharedPtr<Skeleton> LoadSkeleton(const std::string& filePath);
		static SharedPtr<SkinnedMesh> LoadSkinnedMesh(const std::string& filePath);
	};

}