#pragma once

#include <ryml.hpp>
#include <ryml_std.hpp>

#include "Ember/Math/Math.h"

// Generic YAML field helpers. Deliberately free of any asset dependency so headers pulled in by
// AssetManager can use them without an include cycle.
namespace Ember {
	namespace Util {
		// Leaves `out` alone when the key is absent, so the caller's existing value stands in as the
		// default for a file written before that field existed.
		template<typename NodeType, typename T>
		inline static void ReadField(const NodeType& node, const char* key, T& out)
		{
			if (node.has_child(ryml::to_csubstr(key)))
				node[ryml::to_csubstr(key)] >> out;
		}

		inline static void SerializeVector2f(ryml::NodeRef node, const Vector2f& vec)
		{
			node |= ryml::SEQ | ryml::FLOW_SL;
			node.append_child() << vec.x;
			node.append_child() << vec.y;
		}
		inline static void SerializeVector3f(ryml::NodeRef node, const Vector3f& vec)
		{
			node |= ryml::SEQ | ryml::FLOW_SL;
			node.append_child() << vec.x;
			node.append_child() << vec.y;
			node.append_child() << vec.z;
		}
		inline static void SerializeVector4f(ryml::NodeRef node, const Vector4f& vec)
		{
			node |= ryml::SEQ | ryml::FLOW_SL;
			node.append_child() << vec.x;
			node.append_child() << vec.y;
			node.append_child() << vec.z;
			node.append_child() << vec.w;
		}

		inline static void SerializeMatrix4f(ryml::NodeRef node, const Matrix4f& mat)
		{
			node |= ryml::SEQ | ryml::FLOW_SL;
			for (int i = 0; i < 4; i++)
			{
				for (int j = 0; j < 4; j++)
				{
					node.append_child() << mat[i][j];
				}
			}
		}

		template<typename NodeType>
		inline static void DeserializeVector2f(const NodeType& node, Vector2f& vec)
		{
			if (node.is_seq() && node.num_children() == 2)
			{
				node[0] >> vec.x;
				node[1] >> vec.y;
			}
		}

		template<typename NodeType>
		inline static void DeserializeVector3f(const NodeType& node, Vector3f& vec)
		{
			if (node.is_seq() && node.num_children() == 3)
			{
				node[0] >> vec.x;
				node[1] >> vec.y;
				node[2] >> vec.z;
			}
		}

		template<typename NodeType>
		inline static void DeserializeVector4f(const NodeType& node, Vector4f& vec)
		{
			if (node.is_seq() && node.num_children() == 4)
			{
				node[0] >> vec.x;
				node[1] >> vec.y;
				node[2] >> vec.z;
				node[3] >> vec.w;
			}
		}

		template<typename NodeType>
		inline static void DeserializeMatrix4f(const NodeType& node, Matrix4f& mat)
		{
			if (node.is_seq() && node.num_children() == 16)
			{
				for (int i = 0; i < 4; i++)
				{
					for (int j = 0; j < 4; j++)
					{
						node[i * 4 + j] >> mat[i][j];
					}
				}
			}
		}

		// The keyed forms of the above, matching ReadField: absent key leaves `out` untouched.
		template<typename NodeType>
		inline static void ReadVector2f(const NodeType& node, const char* key, Vector2f& out)
		{
			if (node.has_child(ryml::to_csubstr(key)))
				DeserializeVector2f(node[ryml::to_csubstr(key)], out);
		}

		template<typename NodeType>
		inline static void ReadVector3f(const NodeType& node, const char* key, Vector3f& out)
		{
			if (node.has_child(ryml::to_csubstr(key)))
				DeserializeVector3f(node[ryml::to_csubstr(key)], out);
		}

		template<typename NodeType>
		inline static void ReadVector4f(const NodeType& node, const char* key, Vector4f& out)
		{
			if (node.has_child(ryml::to_csubstr(key)))
				DeserializeVector4f(node[ryml::to_csubstr(key)], out);
		}

		template<typename NodeType>
		inline static void ReadMatrix4f(const NodeType& node, const char* key, Matrix4f& out)
		{
			if (node.has_child(ryml::to_csubstr(key)))
				DeserializeMatrix4f(node[ryml::to_csubstr(key)], out);
		}
	}
}
