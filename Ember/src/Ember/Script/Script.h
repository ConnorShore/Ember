#pragma once

#include "Ember/Asset/Asset.h"

#include <sol/sol.hpp>

namespace Ember {

	class Script : public Asset
	{
	public:
		Script(const std::string& filePath);
		Script(UUID uuid, const std::string& name, const std::string& filePath);

		static AssetType GetStaticType() { return AssetType::Script; }
	};

}