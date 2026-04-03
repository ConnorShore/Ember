#include "efpch.h"
#include "UIWidgets.h"

#include <format>

namespace Ember {
	namespace UI {

		bool BeginComboBox(const std::string& id, const std::string& previewValue)
		{
			return ImGui::BeginCombo(std::format("##{}", id).c_str(), previewValue.c_str());
		}

		bool ComboBoxItem(const std::string& label, bool isSelected)
		{
			bool clicked = ImGui::Selectable(label.c_str(), isSelected);

			if (isSelected)
				ImGui::SetItemDefaultFocus();

			return clicked;
		}

		void EndComboBox()
		{
			ImGui::EndCombo();
		}

	}
}