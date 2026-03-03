#include "ebpch.h"
#include "Shader.h"

#include <glad/glad.h>

namespace Ember {
	namespace OpenGL {

		Shader::Shader(const std::string& filePath)
			: m_FilePath(filePath)
		{
			EB_CORE_INFO("Creating shader from file: {}", filePath);

			ShaderProgramSource source = ParseShader();

			m_Id = glCreateProgram();
			unsigned int vs = CompileShader(GL_VERTEX_SHADER, source.VertexSource);
			unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, source.FragmentSource);

			glAttachShader(m_Id, vs);
			glAttachShader(m_Id, fs);

			glLinkProgram(m_Id);
			glValidateProgram(m_Id);

			glDeleteShader(vs);
			glDeleteShader(fs);

			EB_CORE_INFO("Shader created with ID: {}", m_Id);
		}

		Shader::~Shader()
		{
			glDeleteProgram(m_Id);
		}

		void Shader::Bind() const
		{
			glUseProgram(m_Id);
		}

		void Shader::SetVec3f(const std::string& name, const Vector3f& vec)
		{
			int location = glGetUniformLocation(m_Id, name.c_str());
			glUniform3f(location, vec[0], vec[1], vec[2]);
		}

		void Shader::SetVec4f(const std::string& name, const Vector4f& vec)
		{
			int location = glGetUniformLocation(m_Id, name.c_str());
			glUniform4f(location, vec[0], vec[1], vec[2], vec[3]);
		}

		void Shader::SetMat4f(const std::string& name, const Matrix4f& mat)
		{

		}

		const std::string& Shader::GetName() const
		{
			return m_Name;
		}


		unsigned int Shader::CompileShader(unsigned int type, const std::string& source)
		{
			unsigned int id = glCreateShader(type);
			const char* src = source.c_str();
			glShaderSource(id, 1, &src, nullptr);
			glCompileShader(id);

			int result;
			glGetShaderiv(id, GL_COMPILE_STATUS, &result);
			if (result == GL_FALSE)
			{
				int length;
				glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
				char* message = (char*)alloca(length * sizeof(char));
				glGetShaderInfoLog(id, length, &length, message);
				std::cout << "Failed to compile " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << " shader!" << std::endl;
				std::cout << message << std::endl;
				glDeleteShader(id);
				return 0;
			}

			return id;
		}

		ShaderProgramSource Shader::ParseShader()
		{
			std::ifstream stream(m_FilePath);
			if (!stream.is_open())
			{
				EB_CORE_ERROR("Failed to open shader file: {}", m_FilePath);
				return {};
			}

			enum class ShaderType
			{
				NONE = -1, VERTEX = 0, FRAGMENT = 1
			};

			ShaderType type = ShaderType::NONE;
			std::stringstream ss[2];
			std::string line;

			while (getline(stream, line))
			{
				if (line.find("#shader") != std::string::npos)
				{
					if (line.find("vertex") != std::string::npos)
						type = ShaderType::VERTEX;
					else if (line.find("fragment") != std::string::npos)
						type = ShaderType::FRAGMENT;
				}
				else
				{
					ss[(int)type] << line << '\n';
				}
			}

			return { ss[0].str(), ss[1].str() };
		}

	}
}