#pragma once

#include "Ember/Core/ScopedPointer.h"

namespace Ember {

	class GraphicsContext
	{
	public:
		virtual ~GraphicsContext() = default;

		virtual void Init() = 0;
		virtual void SwapBuffers() = 0;

		static ScopedPtr<GraphicsContext> Create(void* window);
	};

}