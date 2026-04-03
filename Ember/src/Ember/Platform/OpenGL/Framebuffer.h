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
			virtual void ViewportResize(uint32_t width, uint32_t height) override;

			int ReadPixel(uint32_t attachmentIndex, int x, int y) const override;
			void ClearAttachment(uint32_t attachmentIndex, int& clearValue) override;

			inline uint32_t GetColorAttachmentID(uint32_t id) const override 
			{
				EB_CORE_ASSERT(id < m_ColorAttachments.size(), "Color attachment id doesn't exist");
				return m_ColorAttachments[id];
			}
			inline uint32_t GetDepthAttachmentID() const override
			{
				return m_DepthAttachment;
			}

			inline const FramebufferSpecification& GetSpecification() const override { return m_Specification; }
			inline uint32_t GetID() const override { return m_Id; }

		private:
			void Regenerate();

		private:
			uint32_t m_Id;
			FramebufferSpecification m_Specification;
			std::vector<uint32_t> m_ColorAttachments;
			uint32_t m_DepthAttachment;
		};

	}
}