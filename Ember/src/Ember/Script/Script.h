#pragma once

#include "Ember/Asset/Asset.h"
#include "ScriptProperty.h"

#include <sol/sol.hpp>
#include <unordered_map>

namespace Ember {

	class Script : public Asset
	{
	public:
		Script(const std::string& filePath);
		Script(UUID uuid, const std::string& name, const std::string& filePath);

		static AssetType GetStaticType() { return AssetType::Script; }

		void SetExposedProperties(const std::unordered_map<std::string, ScriptProperty>& properties) { m_ExposedProperties = properties; }
		const std::unordered_map<std::string, ScriptProperty>& GetExposedProperties() const { return m_ExposedProperties; }

	private:
		std::unordered_map<std::string, ScriptProperty> m_ExposedProperties;
	};
}