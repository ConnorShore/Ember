#pragma once

#include "PostProcessPass.h"
#include "Ember/Render/Shader.h"
#include "Ember/ECS/Types.h"

namespace Ember {

	class OutlinePass : public PostProcessPass
	{
	public:
		OutlinePass() = default;
		virtual ~OutlinePass() = default;

		virtual void Init() override;
		virtual void Render(SharedPtr<Framebuffer> inputBuffer, SharedPtr<Framebuffer> outputBuffer) override;

		void SetSelectedEntityID(EntityID entityID) { m_SelectedEntityID = entityID; }
		void SetGBuffer(SharedPtr<Framebuffer> gBuffer) { m_GBuffer = gBuffer; }
		void SetHdrBuffer(SharedPtr<Framebuffer> hdrBuffer) { m_HdrBuffer = hdrBuffer; }
		void SetOutlineColor(const Vector3f& color) { m_OutlineColor = color; }
		void SetOutlineThickness(float thickness) { m_OutlineThickness = thickness; }

	private:
		SharedPtr<Shader> m_OutlineShader;
		SharedPtr<Framebuffer> m_GBuffer;
		SharedPtr<Framebuffer> m_HdrBuffer;
		EntityID  m_SelectedEntityID;

		Vector3f m_OutlineColor = Vector3f(1.0f, 0.5f, 0.0f);
		float m_OutlineThickness = 1.0f;
	};

}