#pragma once

#include "Ember/Render/Framebuffer.h"

#include <vector>

namespace Ember {
	namespace OpenGL {

		class Framebuffer : public Ember::Framebuffer
		{
		public:
			Framebuffer(const FramebufferSpecification& specification);
			virtual ~Framebuffer();

			void Bind() const override;
			void Unbind() const override;
			virtual void ViewportResize(unsigned int width, unsigned int height) override;

			inline const unsigned int GetColorAttachmentID(unsigned int id) const override 
			{
				EB_CORE_ASSERT(id < m_ColorAttachments.size(), "Color attachment id doesn't exist");
				return m_ColorAttachments[id];
			}
			inline const FramebufferSpecification& GetSpecification() const override { return m_Specification; }

		private:
			void Regenerate();

		private:
			unsigned int m_Id;
			FramebufferSpecification m_Specification;
			std::vector<unsigned int> m_ColorAttachments;
			unsigned int m_DepthAttachment;
		};

	}
}