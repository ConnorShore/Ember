#pragma once

#include <imgui_node_editor.h>
#include <string>
#include <vector>

namespace ne = ax::NodeEditor;

namespace Ember {

	enum class PinType
	{
		Flow,
		Bool,
		Int,
		Float,
		String,
		Object,
		Function,
		Delegate,
	};

	enum class PinKind
	{
		Output,
		Input
	};

	enum class NodeType
	{
		Blueprint,
		Simple,
		Tree,
		Comment,
		Houdini
	};

	struct Node;

	struct Pin
	{
		ne::PinId   ID;
		Node* Node;
		std::string Name;
		PinType     Type;
		PinKind     Kind;

		Pin() = default;
		Pin(int id, const char* name, PinType type) :
			ID(id), Node(nullptr), Name(name), Type(type), Kind(PinKind::Input)
		{
		}
	};

	struct Node
	{
		ne::NodeId ID;
		std::string Name;
		std::vector<Pin> Inputs;
		std::vector<Pin> Outputs;
		ImColor Color;
		NodeType Type;
		ImVec2 Size;

		std::string State;
		std::string SavedState;

		Node() = default;
		Node(int id, const char* name, ImColor color = ImColor(255, 255, 255)) :
			ID(id), Name(name), Color(color), Type(NodeType::Blueprint), Size(0, 0)
		{
		}
	};

	struct Link
	{
		ne::LinkId ID;

		ne::PinId StartPinID;
		ne::PinId EndPinID;

		ImColor Color;

		Link() = default;
		Link(ne::LinkId id, ne::PinId startPinId, ne::PinId endPinId) :
			ID(id), StartPinID(startPinId), EndPinID(endPinId), Color(255, 255, 255)
		{
		}
	};

}