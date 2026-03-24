#pragma once

#include "Panel.h"

#include <string>

namespace Ember {

	class AssetManagerPanel : public Panel
	{
	public:
		AssetManagerPanel(EditorContext* context);
		virtual ~AssetManagerPanel();

		void OnAttach() override;
		void OnImGuiRender() override;

	private:
		static std::string m_AssetDirectory;
		static std::string m_CurrentDirectory;
	};
}