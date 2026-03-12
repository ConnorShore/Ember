#pragma once

#include "Ember/Core/Core.h"
#include "Ember/Math/Math.h"

#include <string>
#include <unordered_map>

namespace Ember {

	//////////////////////////////////////////////////////////////////////////
	// Shader
	//////////////////////////////////////////////////////////////////////////

	class Shader : public SharedResource
	{
	public:
		virtual ~Shader() = default;

		virtual void Bind() const = 0;

		virtual void SetInt(const std::string& name, int value) const = 0;
		virtual void SetFloat(const std::string& name, float value) const = 0;
		virtual void SetFloat2(const std::string& name, const Vector2f& vec) const = 0;
		virtual void SetFloat3(const std::string& name, const Vector3f& vec) const = 0;
		virtual void SetFloat4(const std::string& name, const Vector4f& vec) const = 0;
		virtual void SetMatrix4(const std::string& name, const Matrix4f& mat) const = 0;

		virtual const std::string& GetName() const = 0;

		static SharedPtr<Shader> Create(const std::string& filePath);
		static SharedPtr<Shader> Create(const std::string& name, const std::string& filePath);
	};

	//////////////////////////////////////////////////////////////////////////
	// Shader Library
	//////////////////////////////////////////////////////////////////////////

	class ShaderLibrary
	{
	public:
		const SharedPtr<Shader>& Register(const std::string& filePath);
		const SharedPtr<Shader>& Register(const std::string& name, const std::string& filePath);

		const SharedPtr<Shader>& Get(const std::string& name);
		bool Exists(const std::string& name);

	private:
		void Add(SharedPtr<Shader>&& shader);
		void Add(const std::string& name, SharedPtr<Shader>&& shader);
	private:
		std::unordered_map<std::string, SharedPtr<Shader>> m_ShaderMap;
	};
}
