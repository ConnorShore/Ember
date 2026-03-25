#pragma once

#include "Asset.h"

#include <sol/sol.hpp>

namespace Ember {

	class Script : public Asset
	{
	public:
		Script(const std::string& filePath, sol::table table);
		Script(UUID uuid, const std::string& filePath, sol::table table);

		inline sol::table GetClassTable() { return m_ClassTable; }

		static AssetType GetStaticType() { return AssetType::Script; }

	private:
		sol::table m_ClassTable;
	};

}