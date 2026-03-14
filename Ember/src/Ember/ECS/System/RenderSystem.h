#pragma once

#include "System.h"
#include "Ember/ECS/Registry.h"
#include "ember/Render/Framebuffer.h"
#include "Ember/Render/Shader.h"
#include "Ember/Render/Mesh.h"

namespace Ember {

	class RenderSystem : public System
	{
	public:
		RenderSystem() = default;
		virtual ~RenderSystem() = default;

		void OnAttach(Registry* registry) override;
		void OnDetach(Registry* registry) override;
		void OnUpdate(TimeStep delta, Registry* registry) override;

	private:
		SharedPtr<Framebuffer> m_GBuffer;
		SharedPtr<Mesh> m_ScreenQuad;
	};

}