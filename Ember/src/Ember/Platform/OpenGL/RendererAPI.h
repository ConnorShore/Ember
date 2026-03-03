#pragma once

#include "Ember/Render/RendererAPI.h"

namespace Ember {
	namespace OpenGL {

		class RendererAPI : public Ember::RendererAPI
		{
		public:
			virtual void Clear() override;
			virtual void SetClearColor(Vector4<float> color) override;
			virtual void UseFaceCulling(bool use) override;
			virtual void UseDepthTest(bool use) override;
		};

	}
}