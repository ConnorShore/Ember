#pragma once

#include "Ember/Asset/Asset.h"
#include "ScriptProperty.h"

#include <sol/sol.hpp>
#include <vector>

namespace Ember {

	class Script : public Asset
	{
	public:
		Script(const std::string& filePath);
		Script(UUID uuid, const std::string& name, const std::string& filePath);

		static AssetType GetStaticType() { return AssetType::Script; }

		void SetExposedProperties(const std::vector<ScriptProperty>& properties) { m_ExposedProperties = properties; }
		const std::vector<ScriptProperty>& GetExposedProperties() const { return m_ExposedProperties; }

	private:
		std::vector<ScriptProperty> m_ExposedProperties;
	};
}