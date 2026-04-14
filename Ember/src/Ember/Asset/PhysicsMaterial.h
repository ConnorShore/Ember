#pragma once

#include "Asset.h"

namespace Ember {

	class PhysicsMaterial : public Asset
	{
	public:
		PhysicsMaterial(UUID uuid, const std::string& name, const std::string& filePath)
			: Asset(uuid, name, filePath, AssetType::PhysicsMaterial) {
		}
		PhysicsMaterial(UUID uuid, const std::string& name)
			: PhysicsMaterial(uuid, name, "") {
		}

		float Friction = 0.5f;
		float Bounciness = 0.0f;

		static AssetType GetStaticType() { return AssetType::PhysicsMaterial; }
	};

}