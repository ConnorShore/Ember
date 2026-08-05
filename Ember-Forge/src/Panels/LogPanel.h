#pragma once

#include "Panel.h"

#include <Ember/Core/Logger.h>

namespace Ember {

	class LogPanel : public Panel
	{
	public:
		LogPanel(EditorContext* context)
			: Panel("Log", context) {}
		virtual ~LogPanel() = default;

		void OnImGuiRender() override;

		// Public so the Editor menu can bind a MenuItem check straight to it.
		bool Open = false;

	private:
		struct DisplayLine
		{
			LogRecord Record;
			uint32_t RepeatCount = 1;
		};

		void DrainNewRecords();
		void RebuildFilter();
		bool PassesFilter(const LogRecord& record) const;
		void CopyVisibleToClipboard() const;
		std::string FormatLine(const DisplayLine& line) const;

	private:
		// Panel-side cap; the logger's ring only has to survive until the next drain.
		static constexpr size_t s_MaxLines = 4096;

		std::vector<DisplayLine> m_Lines;
		std::vector<int> m_Filtered;
		std::vector<LogRecord> m_Drained; // Reused so the per-frame drain does not reallocate

		uint64_t m_Cursor = 0;

		ImGuiTextFilter m_TextFilter;
		bool m_LevelEnabled[LogLevelCount] = { true, true, true, true, true };
		bool m_AutoScroll = true;
		bool m_CollapseRepeats = true;
	};

}
