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

		// Depth/Stencil - Depth acts as the sentinel
		Depth,
		DEPTH24STENCIL8 = Depth,	// Any values after this are considered depth/stencil formats
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
		unsigned int Width;
		unsigned int Height;

		FramebufferAttachmentSpecification AttachmentSpecs;
	};

	class Framebuffer : public SharedResource
	{
	public:
		virtual ~Framebuffer() = default;

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;
		virtual void ViewportResize(unsigned int width, unsigned int height) = 0;

		virtual const unsigned int GetColorAttachmentID(unsigned int id) const = 0;
		virtual const FramebufferSpecification& GetSpecification() const = 0;

		static SharedPtr<Framebuffer> Create(const FramebufferSpecification& specification);
	};

}