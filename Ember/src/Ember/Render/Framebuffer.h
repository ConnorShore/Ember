#pragma once

#include "Ember/Core/Core.h"
#include "Ember/Math/Math.h"

#include <vector>

namespace Ember {

	enum class FramebufferTextureFormat
	{
		None = 0,

		// Color
		RGB8,
		RGBA8,
		RG16F,
		RGBA16F,
		RedInteger,

		// Depth acts as a sentinel: any format >= Depth is a depth/stencil attachment
		Depth,
		Depth24,
		Depth32,
		Depth24Stencil8 = Depth,
	};

	static bool IsDepthFormat(FramebufferTextureFormat format)
	{
		return format >= FramebufferTextureFormat::Depth;
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
		uint32_t Width = 1;
		uint32_t Height = 1;
		uint32_t Layers = 1; // For texture arrays and 3D textures

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

		virtual void AttachColorTexture(uint32_t textureId, int mipLevel = 0) = 0;
		virtual void AttachColorTextureLayer(uint32_t textureId, int mipLevel, int layer) = 0;
		virtual void AttachDepthTextureLayer(uint32_t textureId, uint32_t mipLevel, uint32_t layer) = 0;

		virtual int ReadPixel(uint32_t attachmentIndex, int x, int y) const = 0;
		virtual const void* ReadPixels(uint32_t attachmentIndex, int x, int y, uint32_t width, uint32_t height) const = 0;
		
		virtual void ClearAttachment(uint32_t attachmentIndex, int& clearValue) = 0;

		virtual void SetDepthBorderColor(const Vector4f& color) = 0;

		virtual uint32_t GetID() const = 0;

		static SharedPtr<Framebuffer> Create(const FramebufferSpecification& specification);
	};

}