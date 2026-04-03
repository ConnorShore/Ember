#pragma once

#include <string>

namespace Ember {
	namespace UI {

		bool BeginComboBox(const std::string& id, const std::string& previewValue);
		bool ComboBoxItem(const std::string& label, bool isSelected);
		void EndComboBox();

	}
}