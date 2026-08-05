#include "efpch.h"
#include "LogPanel.h"

namespace Ember {

	namespace {

		// Hues follow the console escape codes in Logger.cpp so the two agree at a glance, except
		// Info, which is the bulk of the output and reads better as plain text than as green.
		ImVec4 LevelColor(LogLevel level)
		{
			switch (level)
			{
			case LogLevel::Trace: return ImVec4(0.55f, 0.62f, 0.75f, 1.0f);
			case LogLevel::Info:  return ImVec4(0.86f, 0.88f, 0.90f, 1.0f);
			case LogLevel::Warn:  return ImVec4(0.98f, 0.78f, 0.28f, 1.0f);
			case LogLevel::Error: return ImVec4(0.95f, 0.38f, 0.34f, 1.0f);
			case LogLevel::Fatal: return ImVec4(1.00f, 0.45f, 0.90f, 1.0f);
			default:              return ImVec4(0.86f, 0.88f, 0.90f, 1.0f);
			}
		}

		const char* LevelLabel(LogLevel level)
		{
			switch (level)
			{
			case LogLevel::Trace: return "TRACE";
			case LogLevel::Info:  return "INFO";
			case LogLevel::Warn:  return "WARN";
			case LogLevel::Error: return "ERROR";
			case LogLevel::Fatal: return "FATAL";
			default:              return "INFO";
			}
		}

		// Resolved once, and null when the platform has no tz database so a missing one degrades to
		// UTC instead of throwing out of the render loop.
		const std::chrono::time_zone* LocalZone()
		{
			static const std::chrono::time_zone* s_Zone = []() -> const std::chrono::time_zone*
			{
				try { return std::chrono::current_zone(); }
				catch (...) { return nullptr; }
			}();

			return s_Zone;
		}

	}

	std::string LogPanel::FormatLine(const DisplayLine& line) const
	{
		const auto time = std::chrono::floor<std::chrono::milliseconds>(line.Record.Time);

		std::string timestamp;
		if (const std::chrono::time_zone* zone = LocalZone())
			timestamp = std::format("{:%H:%M:%S}", zone->to_local(time));
		else
			timestamp = std::format("{:%H:%M:%S}", time);

		std::string text = std::format("{}  [{}] {}: {}",
			timestamp,
			LevelLabel(line.Record.Level),
			line.Record.Logger,
			line.Record.Message);

		if (line.RepeatCount > 1)
			text += std::format("  (x{})", line.RepeatCount);

		return text;
	}

	bool LogPanel::PassesFilter(const LogRecord& record) const
	{
		if (!m_LevelEnabled[static_cast<size_t>(record.Level)])
			return false;

		return m_TextFilter.PassFilter(record.Message.c_str());
	}

	void LogPanel::RebuildFilter()
	{
		m_Filtered.clear();

		for (size_t i = 0; i < m_Lines.size(); i++)
		{
			if (PassesFilter(m_Lines[i].Record))
				m_Filtered.push_back(static_cast<int>(i));
		}
	}

	void LogPanel::DrainNewRecords()
	{
		m_Drained.clear();
		Logger::DrainRecords(m_Cursor, m_Drained);

		for (LogRecord& record : m_Drained)
		{
			// Collapsed here rather than in the logger so the file keeps every repeat.
			if (m_CollapseRepeats && !m_Lines.empty()
				&& m_Lines.back().Record.Level == record.Level
				&& m_Lines.back().Record.Message == record.Message)
			{
				m_Lines.back().RepeatCount++;
				m_Lines.back().Record.Time = record.Time;
				continue;
			}

			m_Lines.push_back(DisplayLine{ .Record = std::move(record) });

			if (PassesFilter(m_Lines.back().Record))
				m_Filtered.push_back(static_cast<int>(m_Lines.size() - 1));
		}

		// Trimming a quarter at a time keeps the index rebuild off most frames.
		if (m_Lines.size() > s_MaxLines)
		{
			m_Lines.erase(m_Lines.begin(), m_Lines.begin() + (m_Lines.size() - (s_MaxLines * 3 / 4)));
			RebuildFilter();
		}
	}

	void LogPanel::CopyVisibleToClipboard() const
	{
		std::string buffer;

		for (int index : m_Filtered)
		{
			buffer += FormatLine(m_Lines[index]);
			buffer += '\n';
		}

		ImGui::SetClipboardText(buffer.c_str());
	}

	void LogPanel::OnImGuiRender()
	{
		// Drained even while closed so reopening the window does not show a hole in the history.
		DrainNewRecords();

		if (!Open)
			return;

		if (!ImGui::Begin(m_Title.c_str(), &Open))
		{
			ImGui::End();
			return;
		}

		if (ImGui::Button("Clear"))
		{
			m_Lines.clear();
			m_Filtered.clear();
		}

		ImGui::SameLine();
		if (ImGui::Button("Copy"))
			CopyVisibleToClipboard();

		ImGui::SameLine();
		ImGui::Checkbox("Auto-scroll", &m_AutoScroll);

		ImGui::SameLine();
		ImGui::Checkbox("Collapse", &m_CollapseRepeats);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Fold consecutive identical messages into a single counted line");

		ImGui::SameLine();
		ImGui::SetNextItemWidth(-1.0f);
		bool filterDirty = m_TextFilter.Draw("##LogFilter");

		for (size_t level = 0; level < LogLevelCount; level++)
		{
			if (level > 0)
				ImGui::SameLine();

			ImGui::PushStyleColor(ImGuiCol_Text, LevelColor(static_cast<LogLevel>(level)));
			filterDirty |= ImGui::Checkbox(LevelLabel(static_cast<LogLevel>(level)), &m_LevelEnabled[level]);
			ImGui::PopStyleColor();
		}

		if (filterDirty)
			RebuildFilter();

		ImGui::Separator();

		// Horizontal scrolling rather than wrapping, because the clipper needs a uniform row height.
		if (ImGui::BeginChild("LogScrollingRegion", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar))
		{
			ImGuiListClipper clipper;
			clipper.Begin(static_cast<int>(m_Filtered.size()));

			while (clipper.Step())
			{
				for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
				{
					const DisplayLine& line = m_Lines[m_Filtered[i]];
					const std::string text = FormatLine(line);

					ImGui::PushID(i);
					ImGui::PushStyleColor(ImGuiCol_Text, LevelColor(line.Record.Level));
					ImGui::Selectable(text.c_str());
					ImGui::PopStyleColor();

					if (ImGui::BeginPopupContextItem("##LogLineContext"))
					{
						if (ImGui::MenuItem("Copy Line"))
							ImGui::SetClipboardText(text.c_str());

						ImGui::EndPopup();
					}
					ImGui::PopID();
				}
			}

			if (m_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
				ImGui::SetScrollHereY(1.0f);
		}
		ImGui::EndChild();

		ImGui::End();
	}

}
