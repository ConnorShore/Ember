#pragma once

namespace Ember {

	class GraphNodeUI
	{
	public:
		GraphNodeUI() = default;
		virtual ~GraphNodeUI() = default;

		virtual void Render()
		{
			if (UI::Nodes::BeginExpandableNode(GetName()))
			{
				RenderNodeData();
				UI::Nodes::EndExpandableNode();
			}
		}

		virtual std::string GetName() const = 0;

	protected:
		virtual void RenderNodeData() = 0;
	};

}