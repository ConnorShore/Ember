#pragma once

#include "Ember/Core/Core.h"
#include "Ember/Math/Math.h"
#include "Ember/Asset/Asset.h"

#include "ShaderParser.h"

#include <string>
#include <vector>
#include <unordered_map>

namespace Ember {

	//////////////////////////////////////////////////////////////////////////
	// Shader
	//////////////////////////////////////////////////////////////////////////

	class Shader : public Asset
	{
	public:
		Shader(const std::string& name, const std::string& filePath, const ShaderMacros& macros)
			: Asset(name, filePath, GetStaticType()) {}
		Shader(UUID uuid, const std::string& name, const std::string& filePath, const ShaderMacros& macros)
			: Asset(uuid, name, filePath, GetStaticType()) {
		}

		virtual ~Shader() = default;

		virtual void Bind() const = 0;

		virtual void SetBool(const std::string& name, bool value) const = 0;
		virtual void SetInt(const std::string& name, int value) const = 0;
		virtual int GetInt(const std::string& name) const = 0;
		virtual void SetFloat(const std::string& name, float value) const = 0;
		virtual void SetFloat2(const std::string& name, const Vector2f& vec) const = 0;
		virtual void SetFloat3(const std::string& name, const Vector3f& vec) const = 0;
		virtual void SetFloat4(const std::string& name, const Vector4f& vec) const = 0;
		virtual void SetMatrix4(const std::string& name, const Matrix4f& mat) const = 0;
		virtual void SetMatrix4Array(const std::string& name, const Matrix4f* mats, uint32_t count) const = 0;

		virtual const std::vector<ShaderProperty>& GetProperties() const = 0;

		static AssetType GetStaticType() { return AssetType::Shader; }

		static SharedPtr<Shader> Create(const std::string& filePath, const ShaderMacros& macros = {});
		static SharedPtr<Shader> Create(const std::string& name, const std::string& filePath, const ShaderMacros& macros = {});
		static SharedPtr<Shader> Create(UUID uuid, const std::string& filePath, const ShaderMacros& macros = {});
		static SharedPtr<Shader> Create(UUID uuid, const std::string& name, const std::string& filePath, const ShaderMacros& macros = {});
	};

	//////////////////////////////////////////////////////////////////////////
	// Shader Library
	//////////////////////////////////////////////////////////////////////////

	class ShaderImporter
	{
	public:
		static SharedPtr<Shader> Load(UUID uuid, const std::string& filePath, const ShaderMacros& macros = {})
		{
			return Shader::Create(uuid, filePath, macros);
		}
		static SharedPtr<Shader> Load(UUID uuid, const std::string& name, const std::string& filePath, const ShaderMacros& macros = {})
		{
			return Shader::Create(uuid, name, filePath, macros);
		}
		static SharedPtr<Shader> Load(const std::string& filePath, const ShaderMacros& macros = {})
		{
			return Shader::Create(filePath, macros);
		}
		static SharedPtr<Shader> Load(const std::string& name, const std::string& filePath, const ShaderMacros& macros = {})
		{
			return Shader::Create(name, filePath, macros);
		}
	};
}
