#pragma once

#include "Ember/Core/Core.h"
#include "Ember/Math/Math.h"
#include "Ember/Asset/Asset.h"
#include "Ember/Asset/AssetSerializationMode.h"

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
			: Asset(name, filePath, GetStaticType()), m_Macros(macros) {}
		Shader(UUID uuid, const std::string& name, const std::string& filePath, const ShaderMacros& macros)
			: Asset(uuid, name, filePath, GetStaticType()), m_Macros(macros) {
		}

		virtual ~Shader() = default;

		virtual void Bind() const = 0;

		// Re-parses the shader source from disk and recompiles. Used by the hot-reload system.
		virtual void Reload() = 0;

		// Returns true if the most recent compile/link failed. When true, Bind() falls back to a
		// "missing shader" program (pink) so the error is visually obvious in the viewport.
		virtual bool HasCompileError() const = 0;

		virtual void SetBool(const std::string& name, bool value) const = 0;
		virtual void SetInt(const std::string& name, int value) const = 0;
		virtual void SetIntArray(const std::string& name, const int* values, uint32_t count) const = 0;
		virtual int GetInt(const std::string& name) const = 0;
		virtual void SetFloat(const std::string& name, float value) const = 0;
		virtual void SetFloat2(const std::string& name, const Vector2f& vec) const = 0;
		virtual void SetFloat3(const std::string& name, const Vector3f& vec) const = 0;
		virtual void SetFloat4(const std::string& name, const Vector4f& vec) const = 0;
		virtual void SetMatrix4(const std::string& name, const Matrix4f& mat) const = 0;
		virtual void SetMatrix4Array(const std::string& name, const Matrix4f* mats, uint32_t count) const = 0;

		virtual const std::vector<ShaderProperty>& GetProperties() const = 0;

		const ShaderMacros& GetMacros() const { return m_Macros; }

		static AssetType GetStaticType() { return AssetType::Shader; }

		static SharedPtr<Shader> Create(const std::string& filePath, const ShaderMacros& macros = {});
		static SharedPtr<Shader> Create(const std::string& name, const std::string& filePath, const ShaderMacros& macros = {});
		static SharedPtr<Shader> Create(UUID uuid, const std::string& filePath, const ShaderMacros& macros = {});
		static SharedPtr<Shader> Create(UUID uuid, const std::string& name, const std::string& filePath, const ShaderMacros& macros = {});

	protected:
		ShaderMacros m_Macros;
	};

	//////////////////////////////////////////////////////////////////////////
	// Shader Library
	//////////////////////////////////////////////////////////////////////////

	class ShaderImporter
	{
	public:
		static bool SaveSource(const SharedPtr<Shader>& shader, const std::string& filePath)
		{
			if (!shader)
				return false;
			if (shader->GetFilePath().empty())
				return false;

			std::error_code ec;
			std::filesystem::copy_file(shader->GetFilePath(), filePath, std::filesystem::copy_options::overwrite_existing, ec);
			return !ec;
		}

		static bool SaveCooked(const SharedPtr<Shader>& shader, const std::string& filePath)
		{
			auto cookedPath = std::filesystem::path(filePath);
			cookedPath.replace_extension(".bin");
			return SaveSource(shader, cookedPath.string());
		}

		static bool Save(const SharedPtr<Shader>& shader, const std::string& filePath)
		{
			return SaveSource(shader, filePath);
		}

		static SharedPtr<Shader> LoadSource(UUID uuid, const std::string& filePath, const ShaderMacros& macros = {})
		{
			return Shader::Create(uuid, filePath, macros);
		}

		static SharedPtr<Shader> LoadSource(UUID uuid, const std::string& name, const std::string& filePath, const ShaderMacros& macros = {})
		{
			return Shader::Create(uuid, name, filePath, macros);
		}

		static SharedPtr<Shader> Load(UUID uuid, const std::string& filePath, const ShaderMacros& macros = {})
		{
			switch (AssetSerializationMode::GetRuntimeLoadTier())
			{
			case RuntimeAssetLoadTier::ForceSourceYaml:
				return LoadSource(uuid, filePath, macros);
			case RuntimeAssetLoadTier::ForceCookedBinary:
			{
				auto cookedPath = std::filesystem::path(filePath);
				cookedPath.replace_extension(".bin");
				return Shader::Create(uuid, cookedPath.string(), macros);
			}
			case RuntimeAssetLoadTier::Auto:
			default:
				return Shader::Create(uuid, filePath, macros);
			}
		}
		static SharedPtr<Shader> Load(UUID uuid, const std::string& name, const std::string& filePath, const ShaderMacros& macros = {})
		{
			switch (AssetSerializationMode::GetRuntimeLoadTier())
			{
			case RuntimeAssetLoadTier::ForceSourceYaml:
				return LoadSource(uuid, name, filePath, macros);
			case RuntimeAssetLoadTier::ForceCookedBinary:
			{
				auto cookedPath = std::filesystem::path(filePath);
				cookedPath.replace_extension(".bin");
				return Shader::Create(uuid, name, cookedPath.string(), macros);
			}
			case RuntimeAssetLoadTier::Auto:
			default:
				return Shader::Create(uuid, name, filePath, macros);
			}
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
