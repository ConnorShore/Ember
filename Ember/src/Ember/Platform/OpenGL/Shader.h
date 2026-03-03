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
			virtual ~Shader();
			void Bind() const override;
			void SetVec3f(const std::string& name, const Vector3f& vec) override;
			void SetVec4f(const std::string& name, const Vector4f& vec) override;
			void SetMat4f(const std::string& name, const Matrix4f& mat) override;
			const std::string& GetName() const override;

		private:
			void CompileShader(const std::unordered_map<ShaderType, std::string>& sources);

		private:
			unsigned int m_Id;
			std::string m_Name, m_FilePath;
		};

	}
}