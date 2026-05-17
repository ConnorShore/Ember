#include "ebpch.h"
#include "Shader.h"

#include "Ember/Core/Application.h"
#include "Ember/Core/Constants.h"
#include "Ember/Render/Shader.h"

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

	// Looks up the engine's fallback shader asset and returns its raw GL program id. The
	// fallback is a normal Shader asset loaded by AssetManager::LoadDefaults so it follows the
	// same compile/parse/hot-reload pipeline as every other shader; this helper only exists to
	// avoid recursing through Shader::ActiveProgram() when substituting it for a broken shader.
	static GLuint GetFallbackProgram()
	{
		auto fallback = Ember::Application::Instance().GetAssetManager()
			.GetAsset<Ember::Shader>(Ember::Constants::Assets::FallbackShadUUID);
		if (!fallback)
			return 0;
		return Ember::StaticPointerCast<Ember::OpenGL::Shader>(fallback)->GetRendererId();
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
			if (m_Id != 0)
				glDeleteProgram(m_Id);
		}

		uint32_t Shader::ActiveProgram() const
		{
			return m_Id != 0 ? m_Id : Utils::GetFallbackProgram();
		}

		void Shader::Bind() const
		{
			glUseProgram(ActiveProgram());
		}

		void Shader::Reload()
		{
			EB_CORE_INFO("Reloading shader '{}' from {}", m_Name, m_FilePath);

			// Tear down the old GL program so the new compile starts clean.
			if (m_Id != 0)
			{
				glDeleteProgram(m_Id);
				m_Id = 0;
			}
			m_UniformLocationCache.clear();
			m_Properties.clear();

			ShaderSourceOutput output = ShaderParser::Parse(m_FilePath, m_Macros);
			CompileShader(output.Sources);
			m_Properties = output.Properties;
		}

		void Shader::SetBool(const std::string& name, bool value) const
		{
			glUniform1i(GetUniformLocation(name), value);
		}

		void Shader::SetInt(const std::string& name, int value) const
		{
			glUniform1i(GetUniformLocation(name), value);
		}

		void Shader::SetIntArray(const std::string& name, const int* values, uint32_t count) const
		{
			glUniform1iv(GetUniformLocation(name), count, values);
		}

		int Shader::GetInt(const std::string& name) const
		{
			int location = GetUniformLocation(name);
			if (location == -1)
				return 0;
			int value = 0;
			glGetUniformiv(ActiveProgram(), location, &value);
			return value;
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

		// Compiles each shader stage, attaches to program, links, then cleans up stage objects.
		// On any compile or link failure the GL program is destroyed and m_Id is left at 0 so
		// Bind() will substitute the fallback "missing shader" program.
		void Shader::CompileShader(const ShaderSourceMap& sources)
		{
			EB_CORE_ASSERT(sources.size() <= NUM_SUPPORTED_SHADERS, "Only {} shader types are currently supported!", NUM_SUPPORTED_SHADERS);

			GLuint programId = glCreateProgram();
			std::array<GLuint, NUM_SUPPORTED_SHADERS> shaderIDs{};
			uint32_t shaderIndex = 0;
			bool stageFailed = false;
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

					EB_CORE_ERROR("Failed to compile {} shader '{}'!", ShaderTypeToString(type), m_Name);
					EB_CORE_ERROR("\t{}", message);

					glDeleteShader(shaderId);
					stageFailed = true;
					break;
				}

				glAttachShader(programId, shaderId);
				shaderIDs[shaderIndex++] = shaderId;
			}

			if (stageFailed)
			{
				// Discard the half-built program and any successfully compiled stages so we
				// don't leak GL objects, then leave m_Id at 0 to trigger the fallback program.
				for (uint32_t i = 0; i < shaderIndex; i++)
					glDeleteShader(shaderIDs[i]);
				glDeleteProgram(programId);
				m_Id = 0;
				return;
			}

			glLinkProgram(programId);

			int isLinked;
			glGetProgramiv(programId, GL_LINK_STATUS, &isLinked);
			if (isLinked == GL_FALSE)
			{
				int length;
				glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &length);

				char* message = (char*)_alloca(length * sizeof(char));
				glGetProgramInfoLog(programId, length, &length, message);

				EB_CORE_ERROR("Failed to link shader program: {}", m_Name);
				EB_CORE_ERROR("\t{}", message);

				for (uint32_t i = 0; i < shaderIndex; i++)
					glDeleteShader(shaderIDs[i]);
				glDeleteProgram(programId);
				m_Id = 0;
				return;
			}

			glValidateProgram(programId);

			// Shader objects are no longer needed after linking
			for (uint32_t i = 0; i < shaderIndex; i++)
				glDeleteShader(shaderIDs[i]);

			m_Id = programId;
		}

		// Caches uniform locations to avoid repeated GL queries
		int Shader::GetUniformLocation(const std::string& name) const
		{
			if (m_UniformLocationCache.find(name) != m_UniformLocationCache.end())
				return m_UniformLocationCache[name];

			int location = glGetUniformLocation(ActiveProgram(), name.c_str());
			if (location == -1)
				EB_CORE_WARN("Warning: uniform '{}' doesn't exist!", name);

			m_UniformLocationCache[name] = location;
			return location;
		}
	}
}