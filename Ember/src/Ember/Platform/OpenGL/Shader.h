#pragma once

#include "Ember/Render/Shader.h"

namespace Ember {
	namespace OpenGL {

		struct ShaderProgramSource
		{
			std::string VertexSource;
			std::string FragmentSource;
		};

		class Shader : public Ember::Shader
		{
		public:
			Shader(const std::string& filePath);
			virtual ~Shader();
			void Bind() const override;
			void SetVec3f(const std::string& name, const Vector3f& vec) override;
			void SetVec4f(const std::string& name, const Vector4f& vec) override;
			void SetMat4f(const std::string& name, const Matrix4f& mat) override;
			const std::string& GetName() const override;

		private:
			unsigned int CompileShader(unsigned int type, const std::string& source);
			ShaderProgramSource ParseShader();

		private:
			unsigned int m_Id;
			std::string m_Name, m_FilePath;
		};

	}
}