#pragma once

#include "Layer.h"
#include "ScopedPointer.h"

#include <vector>

#ifdef EB_DEBUG
#include <iostream>
#endif

namespace Ember {

	class LayerStack
	{
	public:
		LayerStack();
		~LayerStack();

		void PushLayer(ScopedPtr<Layer> layer);
		void PushCanvasLayer(ScopedPtr<Layer> canvas);

		void PopLayer(Layer* layer);

		std::vector<ScopedPtr<Layer>>::iterator begin() { return m_Layers.begin(); }
		std::vector<ScopedPtr<Layer>>::iterator end() { return m_Layers.end(); }

	private:
#ifdef EB_DEBUG
		inline void PrintLayerStack()
		{
			std::cout << "----- Layer Stack -----" << std::endl;
			for (auto& layer : m_Layers)
			{
				std::cout << layer->GetName() << std::endl;
			}
			std::cout << "-----------------------" << std::endl;
		}
#else
		inline void PrintLayerStack() {}
#endif

	private:
		std::vector<ScopedPtr<Layer>> m_Layers;
		unsigned int m_LayerPartitionIndex = 0;
	};

}