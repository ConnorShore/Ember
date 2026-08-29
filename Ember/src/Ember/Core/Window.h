#pragma once

#include <string>
#include <functional>

#include "WindowConfig.h"
#include "Ember/Event/Event.h"
#include "Ember/Core/Core.h"
#include "Ember/Core/CursorMode.h"

namespace Ember {

	class Window
	{
	public:
		static constexpr uint32_t MaxWidth = 8192;
		static constexpr uint32_t MaxHeight = 8192;

	public:
		virtual ~Window() = default;

		virtual void PollEvents() = 0;
		virtual void Present() = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		virtual void SetVSync(bool enabled) = 0;
		virtual bool IsVSync() const = 0;

		virtual void SetEventCallback(const std::function<void(Event&)>& callback) = 0;

		virtual void* GetNativeWindow() const = 0;

		virtual void SetCursorPosition(float x, float y) = 0;

		virtual void SetCursorMode(CursorMode mode) = 0;
		virtual CursorMode GetCursorMode() const = 0;

		static ScopedPtr<Window> Create(const WindowConfig& config = WindowConfig());
	};

}