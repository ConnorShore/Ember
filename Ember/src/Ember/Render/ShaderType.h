#pragma once

#include <string_view>

namespace Ember {

#define NUM_SUPPORTED_SHADERS 2

	//////////////////////////////////////////////////////////////////////////
	// Shader Type
	//////////////////////////////////////////////////////////////////////////

	enum class ShaderType
	{
		None = -1,
		Vertex = 0,
		Fragment
	};

	constexpr std::string_view ShaderTypeToString(ShaderType shaderType)
	{
		switch (shaderType)
		{
			case ShaderType::None: return "None";
			case ShaderType::Vertex: return "Vertex";
			case ShaderType::Fragment: return "Fragment";
			default: return "Unknown";
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Shader Data Type
	//////////////////////////////////////////////////////////////////////////

	enum class ShaderDataType
	{
		None = 0,
		Bool,
		Int, Int2, Int3, Int4,
		UInt, UInt2, UInt3, UInt4,
		Float, Float2, Float3, Float4,
		Matrix2, Matrix3, Matrix4,
	};

	constexpr uint32_t ShaderDataTypeSize(ShaderDataType shaderDataType)
	{
		switch (shaderDataType)
		{
		case ShaderDataType::Bool:		return 1;
		case ShaderDataType::Int:		return 4;
		case ShaderDataType::Int2:		return 4 * 2;
		case ShaderDataType::Int3:		return 4 * 3;
		case ShaderDataType::Int4:		return 4 * 4;
		case ShaderDataType::UInt:		return 4;
		case ShaderDataType::UInt2:		return 4 * 2;
		case ShaderDataType::UInt3:		return 4 * 3;
		case ShaderDataType::UInt4:		return 4 * 4;
		case ShaderDataType::Float:		return 4;
		case ShaderDataType::Float2:	return 4 * 2;
		case ShaderDataType::Float3:	return 4 * 3;
		case ShaderDataType::Float4:	return 4 * 4;
		case ShaderDataType::Matrix2:	return 4 * 2 * 2;
		case ShaderDataType::Matrix3:	return 4 * 3 * 3;
		case ShaderDataType::Matrix4:	return 4 * 4 * 4;
		default:						return 0;
		}
	}

	constexpr uint32_t ShaderDataTypeCount(ShaderDataType shaderDataType)
	{
		switch (shaderDataType)
		{
		case ShaderDataType::Bool:		return 1;
		case ShaderDataType::Int:		return 1;
		case ShaderDataType::Int2:		return 2;
		case ShaderDataType::Int3:		return 3;
		case ShaderDataType::Int4:		return 4;
		case ShaderDataType::UInt:		return 1;
		case ShaderDataType::UInt2:		return 2;
		case ShaderDataType::UInt3:		return 3;
		case ShaderDataType::UInt4:		return 4;
		case ShaderDataType::Float:		return 1;
		case ShaderDataType::Float2:	return 2;
		case ShaderDataType::Float3:	return 3;
		case ShaderDataType::Float4:	return 4;
		case ShaderDataType::Matrix2:	return 2 * 2;
		case ShaderDataType::Matrix3:	return 3 * 3;
		case ShaderDataType::Matrix4:	return 4 * 4;
		default:						return 0;
		}
	}
}