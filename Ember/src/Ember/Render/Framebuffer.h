#pragma once

#include "Ember/Core/Core.h"

#include <vector>

namespace Ember {

	enum class FramebufferTextureFormat
	{
		None = 0,

		// Color
		RGBA8,
		RGBA16F,
		RED_INTEGER,

		// Depth acts as a sentinel: any format >= Depth is a depth/stencil attachment
		Depth,
		DEPTH24STENCIL8 = Depth,
	};

	static bool IsDepthFormat(FramebufferTextureFormat format)
	{
		return static_cast<int>(format) >= static_cast<int>(FramebufferTextureFormat::Depth);
	}

	struct FramebufferTextureSpecification
	{
		FramebufferTextureFormat TextureFormat = FramebufferTextureFormat::None;
		// Add filtering/wrapping options later

		FramebufferTextureSpecification() = default;
		FramebufferTextureSpecification(FramebufferTextureFormat format) : TextureFormat(format) {}
	};

	struct FramebufferAttachmentSpecification
	{
		std::vector<FramebufferTextureSpecification> Attachments;

		FramebufferAttachmentSpecification() = default;
		FramebufferAttachmentSpecification(std::initializer_list<FramebufferTextureSpecification> attachments)
			: Attachments(attachments) {
		}
	};

	struct FramebufferSpecification
	{
		uint32_t Width;
		uint32_t Height;

		FramebufferAttachmentSpecification AttachmentSpecs;
	};

	class Framebuffer : public SharedResource
	{
	public:
		virtual ~Framebuffer() = default;

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;
		virtual void ViewportResize(uint32_t width, uint32_t height) = 0;

		virtual uint32_t GetColorAttachmentID(uint32_t id) const = 0;
		virtual uint32_t GetDepthAttachmentID() const = 0;
		virtual const FramebufferSpecification& GetSpecification() const = 0;

		virtual int ReadPixel(uint32_t attachmentIndex, int x, int y) const = 0;
		virtual void ClearAttachment(uint32_t attachmentIndex, int& clearValue) = 0;

		virtual uint32_t GetID() const = 0;

		static SharedPtr<Framebuffer> Create(const FramebufferSpecification& specification);
	};

}