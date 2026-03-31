#pragma once

#include <Ember.h>

#include "Types.h"

namespace Ember {
	namespace UI {

		bool BeginComboBox(const std::string& id, const std::string& previewValue);
		bool ComboBoxItem(const std::string& label, bool isSelected);
		void EndComboBox();

	}
}