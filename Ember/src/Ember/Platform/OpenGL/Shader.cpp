#include "ebpch.h"
#include "Shader.h"

#include <glad/glad.h>

namespace Utils {
	static GLuint GLShaderTypeFromShaderType(Ember::ShaderType type)
	{
		switch (type)
		{
		case Ember::ShaderType::Vertex:		return GL_VERTEX_SHADER;
		case Ember::ShaderType::Fragment:	return GL_FRAGMENT_SHADER;
		case Ember::ShaderType::None:
		default:							EB_CORE_ASSERT(false, "None Shader type is not supported for GL!"); return 0;
		}
	}
}

namespace Ember {
	namespace OpenGL {

		Shader::Shader(UUID uuid, const std::string& name, const std::string& filePath, const ShaderMacros& macros)
			: Ember::Shader(uuid, name, filePath, macros)
		{
			EB_CORE_INFO("Creating shader with name {} from file: {}", m_Name, m_FilePath);

			ShaderSourceOutput output = ShaderParser::Parse(m_FilePath, macros);
			CompileShader(output.Sources);
			m_Properties = output.Properties;

			EB_CORE_INFO("Shader created with ID: {}", m_Id);
		}

		Shader::Shader(const std::string& name, const std::string& filePath, const ShaderMacros& macros)
			: Shader(UUID(), name, filePath, macros)
		{
		}

		Shader::Shader(const std::string& name, const std::string& filePath)
			: Shader(name, filePath, {})
		{
		}

		Shader::Shader(const std::string& filePath, const ShaderMacros& macros)
			: Shader(UUID(), std::filesystem::path(filePath).stem().string(), filePath, macros)
		{
		}

		Shader::Shader(UUID uuid, const std::string& name, const std::string& filePath)
			: Shader(uuid, name, filePath, {})
		{
		}

		Shader::Shader(UUID uuid, const std::string& filePath, const ShaderMacros& macros)
			: Shader(uuid, std::filesystem::path(filePath).stem().string(), filePath, macros)
		{
		}

		Shader::~Shader()
		{
			glDeleteProgram(m_Id);
		}

		void Shader::Bind() const
		{
			glUseProgram(m_Id);
		}

		void Shader::SetBool(const std::string& name, bool value) const
		{
			glUniform1i(GetUniformLocation(name), value);
		}

		void Shader::SetInt(const std::string& name, int value) const
		{
			glUniform1i(GetUniformLocation(name), value);
		}

		void Shader::SetFloat(const std::string& name, float value) const
		{
			glUniform1f(GetUniformLocation(name), value);
		}

		void Shader::SetFloat2(const std::string& name, const Vector2f& vec) const
		{
			glUniform2f(GetUniformLocation(name), vec[0], vec[1]);
		}

		void Shader::SetFloat3(const std::string& name, const Vector3f& vec) const
		{
			glUniform3f(GetUniformLocation(name), vec[0], vec[1], vec[2]);
		}

		void Shader::SetFloat4(const std::string& name, const Vector4f& vec) const
		{
			glUniform4f(GetUniformLocation(name), vec[0], vec[1], vec[2], vec[3]);
		}

		void Shader::SetMatrix4(const std::string& name, const Matrix4f& mat) const
		{
			glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
		}

		void Shader::SetMatrix4Array(const std::string& name, const Matrix4f* mats, uint32_t count) const
		{
			glUniformMatrix4fv(GetUniformLocation(name), count, GL_FALSE, &mats[0][0][0]);
		}

		// Compiles each shader stage, attaches to program, links, then cleans up stage objects
		void Shader::CompileShader(const ShaderSourceMap& sources)
		{
			EB_CORE_ASSERT(sources.size() <= NUM_SUPPORTED_SHADERS, "Only {} shader types are currently supported!", NUM_SUPPORTED_SHADERS);

			GLuint programId = glCreateProgram();
			std::array<GLuint, NUM_SUPPORTED_SHADERS> shaderIDs;
			uint32_t shaderIndex = 0;
			for (auto kv : sources) {
				ShaderType type = kv.first;
				GLuint glType = Utils::GLShaderTypeFromShaderType(kv.first);
				const std::string& source = kv.second;

				GLuint shaderId = glCreateShader(glType);
				const char* src = source.c_str();
				glShaderSource(shaderId, 1, &src, nullptr);
				glCompileShader(shaderId);

				int result;
				glGetShaderiv(shaderId, GL_COMPILE_STATUS, &result);
				if (result == GL_FALSE)
				{
					int length;
					glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &length);

					// Stack-allocate the error message buffer to avoid heap alloc in error path
					char* message = (char*)_alloca(length * sizeof(char));
					glGetShaderInfoLog(shaderId, length, &length, message);

					EB_CORE_ERROR("Failed to compile {} shader!", ShaderTypeToString(type));
					EB_CORE_ERROR("\t{}", message);

					glDeleteShader(shaderId);
					break;
				}

				glAttachShader(programId, shaderId);
				shaderIDs[shaderIndex++] = shaderId;
			}

			// Only set id once shader compilation succeeds
			EB_CORE_ASSERT(programId, "Failed to compile shaders!");
			m_Id = programId;

			glLinkProgram(m_Id);

			int isLinked;
			glGetProgramiv(m_Id, GL_LINK_STATUS, &isLinked);
			if (isLinked == GL_FALSE)
			{
				int length;
				glGetProgramiv(m_Id, GL_INFO_LOG_LENGTH, &length);

				char* message = (char*)_alloca(length * sizeof(char));
				glGetProgramInfoLog(m_Id, length, &length, message);

				EB_CORE_ERROR("Failed to link shader program: {}", m_Name);
				EB_CORE_ERROR("\t{}", message);

				glDeleteProgram(m_Id);
				return;
			}

			glValidateProgram(m_Id);

			// Shader objects are no longer needed after linking
			for (uint32_t i = 0; i < shaderIndex; i++)
				glDeleteShader(shaderIDs[i]);
		}

		// Caches uniform locations to avoid repeated GL queries
		int Shader::GetUniformLocation(const std::string& name) const
		{
			if (m_UniformLocationCache.find(name) != m_UniformLocationCache.end())
				return m_UniformLocationCache[name];

			int location = glGetUniformLocation(m_Id, name.c_str());
			if (location == -1)
				EB_CORE_WARN("Warning: uniform '{}' doesn't exist!", name);

			m_UniformLocationCache[name] = location;
			return location;
		}
	}
}