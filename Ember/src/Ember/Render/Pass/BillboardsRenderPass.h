#pragma once

#include "RenderPass.h"

#include "Ember/Math/Math.h"

namespace Ember {

	struct TransformComponent;
	struct BillboardComponent;

	class BillboardsRenderPass : public RenderPass
	{
	public:
		BillboardsRenderPass() = default;
		virtual ~BillboardsRenderPass() = default;

		virtual void Init() override;
		virtual void Execute(RenderContext& context) override;
		virtual void OnViewportResize(uint32_t width, uint32_t height) override;
		virtual void Shutdown() override;

	private:
		Matrix4f CalculateBillboardTransform(const RenderContext& context, const TransformComponent& transform, const BillboardComponent& billboard);

	private:
		SharedPtr<Shader> m_BillboardShader;
	};

}