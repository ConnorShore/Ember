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
		virtual void Shutdown() override;

	private:
		private Matrix4f CalculateBillboardTransform(const RenderContext& context, const TransformComponent& transform, const BillboardComponent& billboard);

	private:
		SharedPtr<Shader> m_BillboardShader;
	};

}