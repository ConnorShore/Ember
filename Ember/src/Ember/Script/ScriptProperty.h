#pragma once

#include "Ember/Math/Math.h"
#include <string>
#include <sol/sol.hpp>
#include <variant>
#include <vector>
#include <utility>

enum class ScriptPropertyType
{
	Unknown = 0,
	Bool,
	Int,
	Float,
	String,
	Enum,
	Vector3f
};

using ScriptPropertyValue = std::variant<float, int, bool, std::string, Ember::Vector3f>;

struct ScriptProperty
{
	std::string Name;
	ScriptPropertyValue Value;
	ScriptPropertyType Type = ScriptPropertyType::Unknown;

	// Only populated when Type == Enum. Pairs of (option name, option int value).
	// The Value variant holds the currently selected option's int value.
	std::vector<std::pair<std::string, int>> EnumOptions;

	ScriptProperty() = default;
	ScriptProperty(const std::string& name, const ScriptPropertyValue& value, ScriptPropertyType type)
		: Name(name), Value(value), Type(type) {
	}
	ScriptProperty(const std::string& name, const ScriptPropertyValue& value, ScriptPropertyType type,
		std::vector<std::pair<std::string, int>> enumOptions)
		: Name(name), Value(value), Type(type), EnumOptions(std::move(enumOptions)) {
	}
};