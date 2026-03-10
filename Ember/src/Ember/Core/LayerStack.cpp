#include "ebpch.h"
#include "LayerStack.h"
#include "Core.h"

namespace Ember {


	LayerStack::LayerStack()
	{
	}

	LayerStack::~LayerStack()
	{
	}

	void LayerStack::PushLayer(ScopedPtr<Layer> layer)
	{
		layer->OnAttach();

		EB_CORE_TRACE("Pushing layer {} to location {}", layer->GetName(), m_LayerPartitionIndex);
		m_Layers.insert(m_Layers.begin() + (m_LayerPartitionIndex++), std::move(layer));
		PrintLayerStack();
	}

	void LayerStack::PushCanvasLayer(ScopedPtr<Layer> canvas)
	{
		canvas->OnAttach();

		EB_CORE_TRACE("Pushing canvas {} to location {}", canvas->GetName(), m_Layers.size());
		m_Layers.push_back(std::move(canvas));
		PrintLayerStack();
	}

	void LayerStack::PopLayer(Layer* layer)
	{
		auto it = std::ranges::find_if(m_Layers,
			[layer](const ScopedPtr<Layer>& l) {
				return l.Ptr() == layer;
			});

		if (it != m_Layers.end()) {
			layer->OnDetach();

			EB_CORE_TRACE("Removing layer {} from location {}", layer->GetName(), m_LayerPartitionIndex);
			m_Layers.erase(it);
			m_LayerPartitionIndex--;
		}
	}

}