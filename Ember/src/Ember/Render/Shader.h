#pragma once

#include "Ember/Core/Core.h"
#include "Ember/Math/Math.h"

#include <string>
#include <unordered_map>

namespace Ember {

	class Shader : public SharedResource
	{
	public:
		virtual ~Shader() = default;

		virtual void Bind() const = 0;

		virtual void SetVec3f(const std::string& name, const Vector3f& vec) = 0;
		virtual void SetVec4f(const std::string& name, const Vector4f& vec) = 0;
		virtual void SetMat4f(const std::string& name, const Matrix4f& mat) = 0;

		virtual const std::string& GetName() const = 0;

		static SharedPtr<Shader> Create(const std::string& filePath);
	};

	class ShaderLibrary
	{
	public:
		void Add(SharedPtr<Shader> shader);
		SharedPtr<Shader> Get(const std::string& name);
		bool Exists(SharedPtr<Shader> shader);
		SharedPtr<Shader> Load(const std::string& filePath);

	private:
		std::unordered_map<std::string, SharedPtr<Shader>> m_ShaderMap;
	};

}
