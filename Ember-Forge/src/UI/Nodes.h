#pragma once

#include "Types.h"

#include <string>

namespace Ember {
	namespace UI::Nodes {

		bool BeginExpandableNode(const std::string& title, bool showRemove = false, UICallbackFunc onRemoveFunc = nullptr);
		void EndExpandableNode();
	}
}