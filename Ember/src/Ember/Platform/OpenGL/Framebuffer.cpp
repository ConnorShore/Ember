#include "ebpch.h"
#include "Framebuffer.h"

#include "Ember/Core/Window.h"

#include <glad/glad.h>

namespace Ember {
	namespace OpenGL {

		GLuint FramebufferTextureFormatToGLFormat(FramebufferTextureFormat format)
		{
			switch (format)
			{
			case FramebufferTextureFormat::RGBA8:			return GL_RGBA8;
			case FramebufferTextureFormat::RGBA16F:			return GL_RGBA16F;
			case FramebufferTextureFormat::RED_INTEGER:		return GL_R32I;
			case FramebufferTextureFormat::DEPTH24STENCIL8:	return GL_DEPTH24_STENCIL8;
			case FramebufferTextureFormat::None:			return 0;
			default:
				EB_CORE_ASSERT(false, "Unknown FramebufferTextureFormat!");
				return 0;
			}
		}

		GLuint FramebufferAttachmentTypeToGLAttachmentType(FramebufferTextureFormat format, uint32_t index)
		{
			switch (format)
			{
			case FramebufferTextureFormat::RGBA8:
			case FramebufferTextureFormat::RGBA16F:
			case FramebufferTextureFormat::RED_INTEGER:		return GL_COLOR_ATTACHMENT0 + index;
			case FramebufferTextureFormat::DEPTH24STENCIL8:	return GL_DEPTH_STENCIL_ATTACHMENT;
			case FramebufferTextureFormat::None:			return 0;
			default:
				EB_CORE_ASSERT(false, "Unknown FramebufferTextureFormat!");
				return 0;
			}
		}

		Framebuffer::Framebuffer(const FramebufferSpecification& specification)
			: m_Id(0), m_Specification(specification)
		{
			Regenerate();
		}

		Framebuffer::~Framebuffer()
		{
			glDeleteFramebuffers(1, &m_Id);
		}

		void Framebuffer::Bind() const
		{
			glBindFramebuffer(GL_FRAMEBUFFER, m_Id);
		}

		void Framebuffer::Unbind() const
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		void Framebuffer::ViewportResize(uint32_t width, uint32_t height)
		{
			if (width == 0 || height == 0 || width > Window::MaxWidth || height > Window::MaxHeight)
			{
				// Don't render if invalid or too large
				return;
			}

			m_Specification.Width = width;
			m_Specification.Height = height;
			Regenerate();
		}

		// Reads a single pixel from an integer color attachment (used for entity ID picking)
		int Framebuffer::ReadPixel(uint32_t attachmentIndex, int x, int y) const
		{
			glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);
			int pixelData;
			glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &pixelData);
			return pixelData;
		}

		void Framebuffer::ClearAttachment(uint32_t attachmentIndex, int& clearValue)
		{
			glClearNamedFramebufferiv(m_Id, GL_COLOR, attachmentIndex, &clearValue);
		}

		// Tears down existing GPU resources and rebuilds all attachments from the spec.
		// Called on creation and every viewport resize.
		void Framebuffer::Regenerate()
		{
			if (m_Id) {
				glDeleteFramebuffers(1, &m_Id); 
				glDeleteTextures(static_cast<GLsizei>(m_ColorAttachments.size()), m_ColorAttachments.data());
				glDeleteTextures(1, &m_DepthAttachment);

				m_Id = 0;
				m_ColorAttachments.clear();
				m_DepthAttachment = 0;
			}

			glCreateFramebuffers(1, &m_Id);

			std::vector<GLenum> drawBuffers;
			bool containsDepth = false;
			uint32_t size = static_cast<uint32_t>(m_Specification.AttachmentSpecs.Attachments.size());
			for (uint32_t i = 0; i < size; i++)
			{
				auto attachmentSpec = m_Specification.AttachmentSpecs.Attachments[i];
				auto glColorFormat = FramebufferTextureFormatToGLFormat(attachmentSpec.TextureFormat);
				auto glAttachment = FramebufferAttachmentTypeToGLAttachmentType(attachmentSpec.TextureFormat, static_cast<uint32_t>(m_ColorAttachments.size()));

				if (!IsDepthFormat(attachmentSpec.TextureFormat))
				{
					// Create the color texture
					uint32_t textureId;
					glCreateTextures(GL_TEXTURE_2D, 1, &textureId);
					glTextureStorage2D(textureId, 1, glColorFormat, m_Specification.Width, m_Specification.Height);

					glTextureParameteri(textureId, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTextureParameteri(textureId, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTextureParameteri(textureId, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					glTextureParameteri(textureId, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

					glNamedFramebufferTexture(m_Id, glAttachment, textureId, 0);

					drawBuffers.push_back(glAttachment);
					m_ColorAttachments.push_back(textureId);
				}
				else
				{
					// Create the depth/stencil texture
					if (containsDepth) 
					{
						EB_CORE_ASSERT(false, "More than 1 Depth/Stencil attachments specified. Discarding!");
						continue;
					}

					uint32_t depthTextureId;
					glCreateTextures(GL_TEXTURE_2D, 1, &depthTextureId);
					glTextureStorage2D(depthTextureId, 1, glColorFormat, m_Specification.Width, m_Specification.Height);
					glNamedFramebufferTexture(m_Id, glAttachment, depthTextureId, 0);

					m_DepthAttachment = depthTextureId;
					containsDepth = true;
				}
			}

			// Tell OpenGL which color attachments this FBO can write to
			if (!m_ColorAttachments.empty())
				glNamedFramebufferDrawBuffers(m_Id, (GLsizei)drawBuffers.size(), drawBuffers.data());
			else
				glNamedFramebufferDrawBuffers(m_Id, 0, nullptr);

			EB_CORE_ASSERT(glCheckNamedFramebufferStatus(m_Id, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is incomplete!");
		}
	}
}