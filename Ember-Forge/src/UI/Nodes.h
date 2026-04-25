#pragma once

#include "Types.h"

#include <string>

namespace Ember {
	namespace UI::Nodes {

		bool BeginExpandableNode(const std::string& title);
		bool BeginEnabledExpandableNode(const std::string& title, bool& enabled, UICallbackFunc callbackFunc = nullptr);
		bool BeginRemoveableExpandableNode(const std::string& title, UICallbackFunc onRemoveFunc = nullptr);
		void EndExpandableNode();
	}
}