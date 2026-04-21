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

		virtual void SetSelectedEntityID(EntityID entityID) { m_SelectedEntityID = entityID; }
		virtual void SetGBuffer(SharedPtr<Framebuffer> gBuffer) { m_GBuffer = gBuffer; }
		virtual void SetHdrBuffer(SharedPtr<Framebuffer> hdrBuffer) { m_HdrBuffer = hdrBuffer; }
		virtual void SetOutlineColor(const Vector3f& color) { m_OutlineColor = color; }
		virtual void SetOutlineThickness(float thickness) { m_OutlineThickness = thickness; }

		inline virtual PostProcessStage GetStage() const override { return PostProcessStage::HDR; }

	private:
		SharedPtr<Shader> m_OutlineShader;
		SharedPtr<Framebuffer> m_GBuffer;
		SharedPtr<Framebuffer> m_HdrBuffer;
		EntityID  m_SelectedEntityID = Constants::Entities::InvalidEntityID;

		Vector3f m_OutlineColor = Vector3f(1.0f, 0.5f, 0.0f);
		float m_OutlineThickness = 1.0f;
	};

}