#pragma once

#include <Ember.h>

#include "Types.h"

namespace Ember {

	namespace UI::PropertyGrid {

		// Property Grid Layout
		bool Begin(const std::string& id);
		void End();

		// Property Grid Items
		bool HeaderWithActionButton(const std::string& headerLabel, const std::string& buttonLabel);

		// Property Grid Widgets
		bool Slider(const std::string& label, float& value, float min = 0.0f, float max = 0.0f);
		bool Float(const std::string& label, float& value, float step = 0.1f, float min = 0.0f, float max = 0.0f);
		bool Float2(const std::string& label, Vector2f& value, float step = 0.1f, float min = 0.0f, float max = 0.0f);
		bool Float3(const std::string& label, Vector3f& value, float step = 0.1f, float min = 0.0f, float max = 0.0f);
		bool Float4(const std::string& label, Vector4f& value, float step = 0.1f, float min = 0.0f, float max = 0.0f);

		bool Color3(const std::string& label, Vector3f& color);
		bool Color4(const std::string& label, Vector4f& color);

		bool DragDropTexture(const std::string& label, UUID textureID, std::string& outDroppedPath, UICallbackFunc clearButtonFunc);
	}

}