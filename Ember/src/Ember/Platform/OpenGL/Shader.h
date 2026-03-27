#pragma once

#include "Ember/Render/Shader.h"
#include "Ember/Render/ShaderParser.h"

#include <unordered_map>

namespace Ember {
	namespace OpenGL {

		class Shader : public Ember::Shader
		{
		public:
			Shader(const std::string& name, const std::string& filePath, const ShaderMacros& macros);
			Shader(const std::string& name, const std::string& filePath);
			Shader(const std::string& filePath, const ShaderMacros& macros);
			Shader(UUID uuid, const std::string& name, const std::string& filePath, const ShaderMacros& macros);
			Shader(UUID uuid, const std::string& name, const std::string& filePath);
			Shader(UUID uuid, const std::string& filePath, const ShaderMacros& macros);
			virtual ~Shader();

			virtual void Bind() const override;

			virtual void SetBool(const std::string& name, bool value) const override;
			virtual void SetInt(const std::string& name, int value) const override;
			virtual void SetFloat(const std::string& name, float value) const override;
			virtual void SetFloat2(const std::string& name, const Vector2f& vec) const override;
			virtual void SetFloat3(const std::string& name, const Vector3f& vec) const override;
			virtual void SetFloat4(const std::string& name, const Vector4f& vec) const override;
			virtual void SetMatrix4(const std::string& name, const Matrix4f& mat) const override;

			virtual const std::vector<ShaderProperty>& GetProperties() const override { return m_Properties; }

		private:
			void CompileShader(const ShaderSourceMap& sources);
			int GetUniformLocation(const std::string& name) const;

		private:
			unsigned int m_Id;
			mutable std::unordered_map<std::string, int> m_UniformLocationCache;
			std::vector<ShaderProperty> m_Properties;
		};

	}
}