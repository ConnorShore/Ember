#pragma once

#include "Ember/Render/Shader.h"
#include "Ember/Render/ShaderParser.h"

#include <unordered_map>

namespace Ember {
	namespace OpenGL {

		class Shader : public Ember::Shader
		{
		public:
			Shader(const std::string& filePath);
			Shader(const std::string& name, const std::string& filePath);
			virtual ~Shader();

			void Bind() const override;
			void SetFloat3(const std::string& name, const Vector3f& vec) override;
			void SetFloat4(const std::string& name, const Vector4f& vec) override;
			void SetMatrix4(const std::string& name, const Matrix4f& mat) override;

			const std::string& GetName() const override;

		private:
			void CompileShader(const ShaderSourceMap& sources);
			int GetUniformLocation(const std::string& name);

		private:
			unsigned int m_Id;
			std::string m_Name, m_FilePath;

			std::unordered_map<std::string, int> m_UniformLocationCache;
		};

	}
}