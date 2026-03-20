#pragma once

#include "Panel.h"

namespace Ember {

	class AssetManagerPanel : public Panel
	{
	public:
		AssetManagerPanel(EditorContext* context);
		virtual ~AssetManagerPanel();

		void OnImGuiRender() override;

	private:

	};
}