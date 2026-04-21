#pragma once

#include <Ember/Math/Math.h>
#include <Ember/Asset/UUID.h>

#include "Types.h"

#include <string>

namespace Ember {

	namespace UI::PropertyGrid {

		// Property Grid Layout
		bool Begin(const std::string& id);
		void End();

		// Property Grid Items
		bool HeaderWithActionButton(const std::string& headerLabel, const std::string& buttonLabel, const std::string& caption = "");
		bool Checkbox(const std::string& label, bool& value);
		bool InputText(const std::string& label, std::string& value);
		bool DirectoryInput(const std::string& label, std::string& directoryPath, UICallbackFunc browseFunc);

		// Property Grid Widgets
		bool SliderInt(const std::string& label, int& value, int min = 0, int max = 0);
		bool SliderFloat(const std::string& label, float& value, float min = 0.0f, float max = 0.0f);

		bool Int(const std::string& label, int& value, int step = 1, int min = 0, int max = 0);
		bool UInt(const std::string& label, uint32_t& value, uint32_t step = 1, uint32_t min = 0, uint32_t max = 0);

		bool Float(const std::string& label, float& value, float step = 0.1f, float min = 0.0f, float max = 0.0f, const std::string& format = "%.2f");
		bool Float2(const std::string& label, Vector2f& value, float step = 0.1f, float min = 0.0f, float max = 0.0f);
		bool Float3(const std::string& label, Vector3f& value, float step = 0.1f, float min = 0.0f, float max = 0.0f);
		bool Float4(const std::string& label, Vector4f& value, float step = 0.1f, float min = 0.0f, float max = 0.0f);

		bool Color3(const std::string& label, Vector3f& color);
		bool Color4(const std::string& label, Vector4f& color);

		// Returns true if a payload was dropped. 
		// Pass nullptr to browseFunc or clearFunc to hide those respective buttons.
		bool AssetReference(const std::string& label, const std::string& assetName, const std::string& payloadType, std::string& outDroppedPayload, UICallbackFunc browseFunc = nullptr, UICallbackFunc clearFunc = nullptr);

		// A flexible row for action buttons. Leave btn2 empty to draw a single full-width button.
		void ActionRow(const std::string& label, const std::string& btn1Label, UICallbackFunc btn1Func, const std::string& btn2Label = "", UICallbackFunc btn2Func = nullptr);

		// Drag Drop Items
		bool DragDropTexture(const std::string& label, UUID textureID, std::string& outDroppedPath, UICallbackFunc clearButtonFunc);

		// Combo box
		bool BeginComboBox(const std::string& label, const std::string& previewValue);
		bool ComboBoxItem(const std::string& itemLabel, bool isSelected);
		void EndComboBox();
	}

}