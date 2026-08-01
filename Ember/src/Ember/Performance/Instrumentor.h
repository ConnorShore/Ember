// Original Source: https://gist.github.com/TheCherno/31f135eea6ee729ab5f26a6908eb3a5e

#pragma once

#include <string>
#include <chrono>
#include <algorithm>
#include <charconv>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <thread>
#include <mutex>

namespace Ember {

	struct ProfileResult
	{
		// Name points at a string literal / __FUNCSIG__ with static storage duration
		// (see InstrumentationTimer), so it is safe to reference as a raw pointer without
		// copying into a std::string on the hot measurement path.
		const char* Name;
		long long Start, End;
		uint32_t ThreadID;
	};

	struct InstrumentationSession
	{
		std::string Name;
	};

	// Chrome-tracing (chrome://tracing / Perfetto) instrumentor. Events are formatted with std::to_chars
	// into a reusable buffer and block-written as it fills, deliberately avoiding both a per-event flush
	// (~15-25us each, so fine-grained scopes measured the profiler) and buffering everything for one huge
	// drain (~875MB in a single call froze the game for ~22s mid-frame).
	class Instrumentor
	{
	private:
		// Flush the text buffer to disk once it grows past this many bytes (~1MB blocks).
		static constexpr size_t s_FlushChunkBytes = 1u << 20;

		InstrumentationSession* m_CurrentSession;
		std::ofstream m_OutputStream;
		bool m_FirstProfile;
		std::string m_TextBuffer;
		std::mutex m_Mutex;
	public:
		Instrumentor()
			: m_CurrentSession(nullptr), m_FirstProfile(true)
		{
		}

		void BeginSession(const std::string& name, const std::filesystem::path& filepath = "results.json")
		{
			std::lock_guard<std::mutex> lock(m_Mutex);

			if (m_CurrentSession)
				EndSessionInternal();

			const auto parentDir = filepath.parent_path();
			if (!parentDir.empty() && !std::filesystem::exists(parentDir))
			{
				std::error_code ec;
				std::filesystem::create_directories(parentDir, ec);
				if (ec)
				{
					std::cerr << "Failed to create profile directory '" << parentDir.string()
						<< "': " << ec.message() << std::endl;
					return;
				}
			}

			m_OutputStream.open(filepath, std::ios::out | std::ios::trunc);
			if (!m_OutputStream.is_open())
			{
				std::cerr << "Failed to open profile file: " << filepath.string() << std::endl;
				return;
			}

			m_TextBuffer.clear();
			m_TextBuffer.reserve(s_FlushChunkBytes + 4096);
			m_FirstProfile = true;
			WriteHeader();
			m_CurrentSession = new InstrumentationSession{ name };
		}

		void EndSession()
		{
			std::lock_guard<std::mutex> lock(m_Mutex);
			EndSessionInternal();
		}

		void WriteProfile(const ProfileResult& result)
		{
			std::lock_guard<std::mutex> lock(m_Mutex);
			if (!m_CurrentSession)
				return;

			AppendEvent(result);

			if (m_TextBuffer.size() >= s_FlushChunkBytes)
				FlushTextBuffer();
		}

		static Instrumentor& Get()
		{
			static Instrumentor instance;
			return instance;
		}

	private:
		// The following helpers all assume m_Mutex is already held by the caller.

		void EndSessionInternal()
		{
			if (!m_CurrentSession)
				return;

			if (m_OutputStream.is_open())
			{
				FlushTextBuffer();
				WriteFooter();
				m_OutputStream.flush();
				m_OutputStream.close();
			}
			m_TextBuffer.clear();

			delete m_CurrentSession;
			m_CurrentSession = nullptr;
			m_FirstProfile = true;
		}

		template<typename T>
		void AppendInt(T value)
		{
			char buf[24];
			auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), value);
			m_TextBuffer.append(buf, static_cast<size_t>(ptr - buf));
		}

		// Formats one event into m_TextBuffer. Reuses the buffer's capacity, so there is no
		// per-event heap allocation, and uses to_chars rather than iostream for the numbers.
		void AppendEvent(const ProfileResult& result)
		{
			if (!m_FirstProfile)
				m_TextBuffer += ',';
			m_FirstProfile = false;

			m_TextBuffer += "{\"cat\":\"function\",\"dur\":";
			AppendInt(result.End - result.Start);
			m_TextBuffer += ",\"name\":\"";

			// Escape embedded double-quotes on the fly (matches the previous behavior).
			for (const char* c = result.Name; c && *c; ++c)
				m_TextBuffer += (*c == '"' ? '\'' : *c);

			m_TextBuffer += "\",\"ph\":\"X\",\"pid\":0,\"tid\":";
			AppendInt(result.ThreadID);
			m_TextBuffer += ",\"ts\":";
			AppendInt(result.Start);
			m_TextBuffer += '}';
		}

		// Block-writes the accumulated text and clears it. No flush() — the OS absorbs the
		// write and we flush once at EndSession.
		void FlushTextBuffer()
		{
			if (m_OutputStream.is_open() && !m_TextBuffer.empty())
				m_OutputStream.write(m_TextBuffer.data(), static_cast<std::streamsize>(m_TextBuffer.size()));
			m_TextBuffer.clear();
		}

		void WriteHeader()
		{
			m_OutputStream << "{\"otherData\": {},\"traceEvents\":[";
		}

		void WriteFooter()
		{
			m_OutputStream << "]}";
		}
	};

	class InstrumentationTimer
	{
	public:
		InstrumentationTimer(const char* name)
			: m_Name(name), m_Stopped(false)
		{
			m_StartTimepoint = std::chrono::high_resolution_clock::now();
		}

		~InstrumentationTimer()
		{
			if (!m_Stopped)
				Stop();
		}

		void Stop()
		{
			auto endTimepoint = std::chrono::high_resolution_clock::now();

			long long start = std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimepoint).time_since_epoch().count();
			long long end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch().count();

			uint32_t threadID = std::hash<std::thread::id>{}(std::this_thread::get_id());
			Instrumentor::Get().WriteProfile({ m_Name, start, end, threadID });

			m_Stopped = true;
		}
	private:
		const char* m_Name;
		std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTimepoint;
		bool m_Stopped;
	};

}
