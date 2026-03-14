#pragma once

#include "Ember/Core/Core.h"
#include "Ember/Math/Math.h"
#include "Ember/Asset/Asset.h"

#include "ShaderParser.h"

#include <string>
#include <unordered_map>

namespace Ember {

	//////////////////////////////////////////////////////////////////////////
	// Shader
	//////////////////////////////////////////////////////////////////////////

	class Shader : public Asset
	{
	public:
		Shader(const std::string& name, const std::string& filePath, const ShaderMacros& macros)
			: Asset(name, filePath, AssetType::Shader) {}

		virtual ~Shader() = default;

		virtual void Bind() const = 0;

		virtual void SetInt(const std::string& name, int value) const = 0;
		virtual void SetFloat(const std::string& name, float value) const = 0;
		virtual void SetFloat2(const std::string& name, const Vector2f& vec) const = 0;
		virtual void SetFloat3(const std::string& name, const Vector3f& vec) const = 0;
		virtual void SetFloat4(const std::string& name, const Vector4f& vec) const = 0;
		virtual void SetMatrix4(const std::string& name, const Matrix4f& mat) const = 0;

		static SharedPtr<Shader> Create(const std::string& filePath, const ShaderMacros& macros = {});
		static SharedPtr<Shader> Create(const std::string& name, const std::string& filePath, const ShaderMacros& macros = {});
	};

	//////////////////////////////////////////////////////////////////////////
	// Shader Library
	//////////////////////////////////////////////////////////////////////////

	class ShaderLibrary
	{
	public:
		const SharedPtr<Shader>& Register(const std::string& filePath, const ShaderMacros& macros = {});
		const SharedPtr<Shader>& Register(const std::string& name, const std::string& filePath, const ShaderMacros& macros = {});

		const SharedPtr<Shader>& Get(const std::string& name);
		bool Exists(const std::string& name);

	private:
		void Add(SharedPtr<Shader>&& shader);
		void Add(const std::string& name, SharedPtr<Shader>&& shader);
	private:
		std::unordered_map<std::string, SharedPtr<Shader>> m_ShaderMap;
	};
}
