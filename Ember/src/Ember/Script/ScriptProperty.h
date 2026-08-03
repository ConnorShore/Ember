#pragma once

#include "Ember/Asset/UUID.h"
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
	Vector3f,
	EntityRef,
	AssetRef,
	ReferenceArray
};

enum class ScriptReferenceKind
{
	None = 0,
	Entity,
	Asset,
	Texture,
	Mesh,
	Model,
	Script,
	Shader,
	Material,
	PhysicsMaterial,
	Prefab,
	Font,
	AudioClip,
	Scene
};

inline const char* ScriptReferenceKindToString(ScriptReferenceKind kind)
{
	switch (kind)
	{
	case ScriptReferenceKind::Entity: return "Entity";
	case ScriptReferenceKind::Asset: return "Asset";
	case ScriptReferenceKind::Texture: return "Texture";
	case ScriptReferenceKind::Mesh: return "Mesh";
	case ScriptReferenceKind::Model: return "Model";
	case ScriptReferenceKind::Script: return "Script";
	case ScriptReferenceKind::Shader: return "Shader";
	case ScriptReferenceKind::Material: return "Material";
	case ScriptReferenceKind::PhysicsMaterial: return "PhysicsMaterial";
	case ScriptReferenceKind::Prefab: return "Prefab";
	case ScriptReferenceKind::Font: return "Font";
	case ScriptReferenceKind::AudioClip: return "AudioClip";
	case ScriptReferenceKind::Scene: return "Scene";
	default: return "None";
	}
}

inline ScriptReferenceKind ScriptReferenceKindFromString(const std::string& kind)
{
	if (kind == "Entity") return ScriptReferenceKind::Entity;
	if (kind == "Asset") return ScriptReferenceKind::Asset;
	if (kind == "Texture") return ScriptReferenceKind::Texture;
	if (kind == "Mesh") return ScriptReferenceKind::Mesh;
	if (kind == "Model") return ScriptReferenceKind::Model;
	if (kind == "Script") return ScriptReferenceKind::Script;
	if (kind == "Shader") return ScriptReferenceKind::Shader;
	if (kind == "Material") return ScriptReferenceKind::Material;
	if (kind == "PhysicsMaterial") return ScriptReferenceKind::PhysicsMaterial;
	if (kind == "Prefab") return ScriptReferenceKind::Prefab;
	if (kind == "Font") return ScriptReferenceKind::Font;
	if (kind == "AudioClip") return ScriptReferenceKind::AudioClip;
	if (kind == "Scene") return ScriptReferenceKind::Scene;
	return ScriptReferenceKind::None;
}

// The vector alternative backs ReferenceArray; append new alternatives rather than inserting.
using ScriptPropertyValue = std::variant<float, int, bool, std::string, Ember::Vector3f, Ember::UUID,
	std::vector<Ember::UUID>>;

struct ScriptProperty
{
	std::string Name;
	ScriptPropertyValue Value;
	ScriptPropertyType Type = ScriptPropertyType::Unknown;
	ScriptReferenceKind ReferenceKind = ScriptReferenceKind::None;

	// Only populated when Type == Enum. Pairs of (option name, option int value).
	// The Value variant holds the currently selected option's int value.
	std::vector<std::pair<std::string, int>> EnumOptions;

	ScriptProperty() = default;
	ScriptProperty(const std::string& name, const ScriptPropertyValue& value, ScriptPropertyType type)
		: Name(name), Value(value), Type(type) {
	}
	ScriptProperty(const std::string& name, const ScriptPropertyValue& value, ScriptPropertyType type,
		ScriptReferenceKind referenceKind)
		: Name(name), Value(value), Type(type), ReferenceKind(referenceKind) {
	}
	ScriptProperty(const std::string& name, const ScriptPropertyValue& value, ScriptPropertyType type,
		std::vector<std::pair<std::string, int>> enumOptions)
		: Name(name), Value(value), Type(type), EnumOptions(std::move(enumOptions)) {
	}
};