#pragma once

#include "Ember/Core/Core.h"
#include "Ember/Render/RenderContext.h"
#include "Ember/Render/Framebuffer.h"

#include <unordered_map>
#include <string>

namespace Ember {

	class RenderPass : public SharedResource
	{
	public:
		virtual ~RenderPass() = default;

		virtual void Init() = 0;
		virtual void Execute(RenderContext& context) = 0;
		virtual void OnViewportResize(uint32_t width, uint32_t height) = 0;
		virtual void Shutdown() = 0;

		virtual void SetTextureInput(const std::string& name, uint32_t textureID)
		{
			m_TextureInputs[name] = textureID;
		}

		virtual uint32_t GetTextureOutput(const std::string& name) const
		{
			EB_CORE_ASSERT(m_TextureOutputs.find(name) != m_TextureOutputs.end(), "Texture output with name {} not found!", name);
			return m_TextureOutputs.at(name);
		}

		virtual void SetFramebufferInput(const std::string& name, const SharedPtr<Framebuffer>& framebuffer)
		{
			m_FramebufferInputs[name] = framebuffer;
		}

		virtual SharedPtr<Framebuffer> GetFramebufferOutput(const std::string& name) const
		{
			EB_CORE_ASSERT(m_FramebufferOutputs.find(name) != m_FramebufferOutputs.end(), "Framebuffer output with name {} not found!", name);
			return m_FramebufferOutputs.at(name);
		}

	protected:
		std::unordered_map<std::string, uint32_t> m_TextureInputs;
		std::unordered_map<std::string, uint32_t> m_TextureOutputs;

		std::unordered_map<std::string, SharedPtr<Framebuffer>> m_FramebufferInputs;
		std::unordered_map<std::string, SharedPtr<Framebuffer>> m_FramebufferOutputs;
	};
}