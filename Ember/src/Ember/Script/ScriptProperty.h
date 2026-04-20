#pragma once

#include <string>
#include <sol/sol.hpp>
#include <variant>

enum class ScriptPropertyType
{
	Unknown = 0,
	Bool,
	Int,
	Float,
	String
};

using ScriptPropertyValue = std::variant<float, int, bool, std::string>;

struct ScriptProperty
{
	std::string Name;
	ScriptPropertyValue Value;
	ScriptPropertyType Type = ScriptPropertyType::Unknown;

	ScriptProperty() = default;
	ScriptProperty(const std::string& name, const ScriptPropertyValue& value, ScriptPropertyType type)
		: Name(name), Value(value), Type(type) {
	}
};