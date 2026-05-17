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
			virtual void Reload() override;
			virtual bool HasCompileError() const override { return m_Id == 0; }

			virtual void SetBool(const std::string& name, bool value) const override;
			virtual void SetInt(const std::string& name, int value) const override;
			virtual void SetIntArray(const std::string& name, const int* values, uint32_t count) const override;
			virtual int GetInt(const std::string& name) const override;
			virtual void SetFloat(const std::string& name, float value) const override;
			virtual void SetFloat2(const std::string& name, const Vector2f& vec) const override;
			virtual void SetFloat3(const std::string& name, const Vector3f& vec) const override;
			virtual void SetFloat4(const std::string& name, const Vector4f& vec) const override;
			virtual void SetMatrix4(const std::string& name, const Matrix4f& mat) const override;
			virtual void SetMatrix4Array(const std::string& name, const Matrix4f* mats, uint32_t count) const override;

			virtual const std::vector<ShaderProperty>& GetProperties() const override { return m_Properties; }

			// Raw GL program id (0 if compilation failed). Prefer Bind()/ActiveProgram() for normal
			// use; this exists so the fallback-shader lookup can read the program id without
			// recursing back through ActiveProgram().
			uint32_t GetRendererId() const { return m_Id; }

		private:
			void CompileShader(const ShaderSourceMap& sources);
			int GetUniformLocation(const std::string& name) const;
			// Returns m_Id when compilation succeeded, otherwise the shared "missing shader"
			// fallback program so the user can still see something (pink) when their shader is broken.
			uint32_t ActiveProgram() const;

		private:
			uint32_t m_Id = 0;
			mutable std::unordered_map<std::string, int> m_UniformLocationCache;
			std::vector<ShaderProperty> m_Properties;
		};

	}
}