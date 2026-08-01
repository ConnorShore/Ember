#pragma once

// Small, dependency-free test framework - no engine, no vendored lib. Engine-typed helpers
// (Vector3f/Matrix4f comparisons, scene fixtures) live in TestHelpers.h.
//
// EB_CHECK* throws and aborts the current test; EB_EXPECT* records the failure and continues.
// RunAll() returns the failed-test count for use as a process exit code. See README.md.

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>
#include <vector>

namespace Ember::Test {

	// The categories a game engine wants coverage in. Filterable at run time so CI can, e.g., run only
	// the fast unit tests on every commit and the slower visual/performance tests nightly.
	namespace Type {
		inline constexpr const char* Unit = "unit";               // pure logic, no engine subsystems
		inline constexpr const char* Integration = "integration"; // multiple subsystems together, no rendering
		inline constexpr const char* Visual = "visual";           // renders a frame and compares pixels
		inline constexpr const char* Performance = "performance"; // asserts on a timing/perf budget
		inline constexpr const char* Stress = "stress";           // long/repeated runs hunting leaks & drift
	}

	// Thrown by EB_CHECK* to abort the current test.
	struct TestFailure { std::string Message; };
	// Thrown by EB_SKIP to mark a test as skipped rather than failed (missing optional asset, etc).
	struct TestSkipped { std::string Reason; };

	struct TestCase {
		const char* Suite;
		const char* Name;
		const char* TestType;
		std::function<void()> Fn;
	};

	enum class Outcome { Passed, Failed, Skipped };

	struct TestResult {
		std::string Suite;
		std::string Name;
		std::string TestType;
		Outcome Result = Outcome::Passed;
		double DurationMs = 0.0;
		std::vector<std::string> Failures; // hard failure last, soft failures in order
		std::string SkipReason;
	};

	//////////////////////////////////////////////////////////////////////////
	// Per-test scratch state: soft failures and free-form notes
	//////////////////////////////////////////////////////////////////////////

	class Context
	{
	public:
		static Context& Get() { static Context s_Instance; return s_Instance; }

		void Begin() { m_SoftFailures.clear(); m_Notes.clear(); }

		void AddSoftFailure(std::string message) { m_SoftFailures.push_back(std::move(message)); }
		const std::vector<std::string>& SoftFailures() const { return m_SoftFailures; }

		// Notes are printed underneath the test line whether it passed or failed. Use them for the
		// measured numbers a human wants to see even on a pass (frame times, entity counts, ...).
		void AddNote(std::string note) { m_Notes.push_back(std::move(note)); }
		const std::vector<std::string>& Notes() const { return m_Notes; }

	private:
		std::vector<std::string> m_SoftFailures;
		std::vector<std::string> m_Notes;
	};

	// Function-local static (Meyers singleton) so registration order across translation units is safe.
	class Registry {
	public:
		static Registry& Get() { static Registry s_Instance; return s_Instance; }
		void Add(const TestCase& tc) { m_Tests.push_back(tc); }
		const std::vector<TestCase>& Tests() const { return m_Tests; }
	private:
		std::vector<TestCase> m_Tests;
	};

	struct AutoRegister {
		AutoRegister(const char* suite, const char* name, const char* type, std::function<void()> fn) {
			Registry::Get().Add(TestCase{ suite, name, type, std::move(fn) });
		}
	};

	inline std::string FormatLocation(const char* file, int line)
	{
		// Trim to the filename; full paths make the output unreadable in a terminal.
		const char* slash = std::strrchr(file, '\\');
		if (!slash) slash = std::strrchr(file, '/');
		const char* shortName = slash ? slash + 1 : file;
		return std::string(shortName) + "(" + std::to_string(line) + ")";
	}

	inline std::string BuildFailureMessage(const char* expr, const char* file, int line, const std::string& extra)
	{
		std::string msg = FormatLocation(file, line) + ": " + expr;
		if (!extra.empty())
			msg += "  -> " + extra;
		return msg;
	}

	[[noreturn]] inline void ReportFail(const char* expr, const char* file, int line, const std::string& extra = {}) {
		throw TestFailure{ BuildFailureMessage(expr, file, line, extra) };
	}

	inline void ReportSoftFail(const char* expr, const char* file, int line, const std::string& extra = {}) {
		Context::Get().AddSoftFailure(BuildFailureMessage(expr, file, line, extra));
	}

	[[noreturn]] inline void ReportSkip(const std::string& reason) {
		throw TestSkipped{ reason };
	}

	//////////////////////////////////////////////////////////////////////////
	// Benchmarking
	//////////////////////////////////////////////////////////////////////////

	struct BenchmarkResult
	{
		double MinMs = 0.0;
		double MedianMs = 0.0;
		double MeanMs = 0.0;
		double MaxMs = 0.0;
		int Iterations = 0;

		std::string ToString() const
		{
			char buffer[256];
			std::snprintf(buffer, sizeof(buffer), "median %.3f ms (min %.3f, mean %.3f, max %.3f) over %d iters",
				MedianMs, MinMs, MeanMs, MaxMs, Iterations);
			return std::string(buffer);
		}
	};

	// Runs `fn` `warmup` times untimed (to page in memory, warm caches, let lazy init settle), then
	// `iterations` times timed. Reports the MEDIAN as the headline number: unlike the mean it shrugs off
	// the occasional OS scheduling hiccup, which is what makes single-shot timings so flaky on a desktop.
	inline BenchmarkResult Benchmark(const std::function<void()>& fn, int iterations = 20, int warmup = 3)
	{
		for (int i = 0; i < warmup; ++i)
			fn();

		std::vector<double> samples;
		samples.reserve(iterations);
		for (int i = 0; i < iterations; ++i)
		{
			const auto start = std::chrono::high_resolution_clock::now();
			fn();
			const auto end = std::chrono::high_resolution_clock::now();
			samples.push_back(std::chrono::duration<double, std::milli>(end - start).count());
		}

		std::sort(samples.begin(), samples.end());

		BenchmarkResult result;
		result.Iterations = iterations;
		result.MinMs = samples.front();
		result.MaxMs = samples.back();
		result.MedianMs = samples[samples.size() / 2];

		double total = 0.0;
		for (double s : samples)
			total += s;
		result.MeanMs = total / static_cast<double>(samples.size());
		return result;
	}

	// Perf budgets are hardware-dependent. EMBER_TEST_PERF_SCALE multiplies every budget so a slower CI
	// box (or a Debug build) can run the same tests without editing numbers into the source.
	inline double PerfScale()
	{
		static double s_Scale = []() {
			const char* raw = std::getenv("EMBER_TEST_PERF_SCALE");
			if (!raw)
				return 1.0;
			const double parsed = std::atof(raw);
			return parsed > 0.0 ? parsed : 1.0;
			}();
		return s_Scale;
	}

	// Collected across the run and optionally dumped to CSV so budgets can be tracked over time
	// rather than only pass/fail'd. Set EMBER_TEST_PERF_CSV=Profiles/perf.csv to emit it.
	struct PerfSample
	{
		std::string Suite;
		std::string Name;
		std::string Label;
		BenchmarkResult Result;
		double BudgetMs = 0.0;
	};

	inline std::vector<PerfSample>& PerfSamples()
	{
		static std::vector<PerfSample> s_Samples;
		return s_Samples;
	}

	//////////////////////////////////////////////////////////////////////////
	// Runner
	//////////////////////////////////////////////////////////////////////////

	struct RunOptions
	{
		const char* TypeFilter = nullptr;  // exact match against TestCase::TestType
		std::string NameFilter;            // case-sensitive substring match against "Suite::Name"
		int Repeat = 1;                    // run the whole selection N times (flakiness hunting)
		std::string XmlOutputPath;         // JUnit XML for CI, empty to skip
		std::string PerfCsvPath;           // perf CSV, empty to skip
		std::string BreadcrumbPath;        // per-test progress log that survives a hard crash
	};

	namespace Detail {

		// Output paths are relative to the working directory (the workspace root). Create the parent
		// directory so pointing --xml at a folder that doesn't exist yet silently works.
		inline void EnsureParentDirectory(const std::string& path)
		{
			std::error_code ec;
			const auto parent = std::filesystem::path(path).parent_path();
			if (!parent.empty())
				std::filesystem::create_directories(parent, ec);
		}

		inline std::string XmlEscape(const std::string& in)
		{
			std::string out;
			out.reserve(in.size());
			for (char c : in)
			{
				switch (c)
				{
				case '&':  out += "&amp;"; break;
				case '<':  out += "&lt;"; break;
				case '>':  out += "&gt;"; break;
				case '"':  out += "&quot;"; break;
				case '\'': out += "&apos;"; break;
				default:   out += c; break;
				}
			}
			return out;
		}

		inline void WriteJUnitXml(const std::string& path, const std::vector<TestResult>& results)
		{
			EnsureParentDirectory(path);
			std::ofstream file(path);
			if (!file.is_open())
			{
				std::printf("  [warn] could not open '%s' for XML output\n", path.c_str());
				return;
			}

			int failures = 0, skipped = 0;
			double totalSeconds = 0.0;
			for (const auto& r : results)
			{
				if (r.Result == Outcome::Failed) ++failures;
				if (r.Result == Outcome::Skipped) ++skipped;
				totalSeconds += r.DurationMs / 1000.0;
			}

			file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
			file << "<testsuite name=\"Ember-Test\" tests=\"" << results.size()
				<< "\" failures=\"" << failures
				<< "\" skipped=\"" << skipped
				<< "\" time=\"" << totalSeconds << "\">\n";

			for (const auto& r : results)
			{
				file << "  <testcase classname=\"" << XmlEscape(r.Suite)
					<< "\" name=\"" << XmlEscape(r.Name)
					<< "\" time=\"" << (r.DurationMs / 1000.0) << "\">\n";

				if (r.Result == Outcome::Failed)
				{
					std::string combined;
					for (const auto& f : r.Failures)
						combined += f + "\n";
					file << "    <failure message=\"assertion failed\">" << XmlEscape(combined) << "</failure>\n";
				}
				else if (r.Result == Outcome::Skipped)
				{
					file << "    <skipped message=\"" << XmlEscape(r.SkipReason) << "\"/>\n";
				}
				file << "  </testcase>\n";
			}
			file << "</testsuite>\n";
		}

		inline void WritePerfCsv(const std::string& path, const std::vector<PerfSample>& samples)
		{
			EnsureParentDirectory(path);
			std::ofstream file(path);
			if (!file.is_open())
			{
				std::printf("  [warn] could not open '%s' for perf CSV output\n", path.c_str());
				return;
			}

			file << "suite,test,label,median_ms,min_ms,mean_ms,max_ms,iterations,budget_ms\n";
			for (const auto& s : samples)
			{
				file << s.Suite << ',' << s.Name << ',' << s.Label << ','
					<< s.Result.MedianMs << ',' << s.Result.MinMs << ',' << s.Result.MeanMs << ','
					<< s.Result.MaxMs << ',' << s.Result.Iterations << ',' << s.BudgetMs << '\n';
			}
		}

		inline bool Matches(const TestCase& test, const RunOptions& options)
		{
			if (options.TypeFilter && std::string(options.TypeFilter) != test.TestType)
				return false;

			if (!options.NameFilter.empty())
			{
				const std::string fullName = std::string(test.Suite) + "::" + test.Name;
				if (fullName.find(options.NameFilter) == std::string::npos)
					return false;
			}
			return true;
		}

	} // namespace Detail

	// The currently-running test, so EB_BENCH can tag its perf samples without extra plumbing.
	inline const TestCase*& CurrentTest()
	{
		static const TestCase* s_Current = nullptr;
		return s_Current;
	}

	inline int RunAll(const RunOptions& options)
	{
		std::vector<TestResult> results;
		int passed = 0, failed = 0, skipped = 0, filteredOut = 0;

		// Breadcrumb log: the name of each test is written and FLUSHED before the test body runs. If a
		// test hard-crashes the process (engine assert -> __debugbreak, access violation, rp3d abort)
		// there is no stack in the console, but the last "RUNNING" line in this file names the culprit.
		std::ofstream breadcrumbs;
		if (!options.BreadcrumbPath.empty())
		{
			Detail::EnsureParentDirectory(options.BreadcrumbPath);
			breadcrumbs.open(options.BreadcrumbPath, std::ios::trunc);
		}

		std::printf("\n================ Ember-Test ================\n");
		if (options.TypeFilter)
			std::printf("  type filter : %s\n", options.TypeFilter);
		if (!options.NameFilter.empty())
			std::printf("  name filter : %s\n", options.NameFilter.c_str());
		if (options.Repeat > 1)
			std::printf("  repeat      : %dx\n", options.Repeat);
		if (PerfScale() != 1.0)
			std::printf("  perf scale  : %.2fx\n", PerfScale());
		std::printf("--------------------------------------------\n");

		const auto suiteStart = std::chrono::high_resolution_clock::now();

		for (int pass = 0; pass < options.Repeat; ++pass)
		{
			if (options.Repeat > 1)
				std::printf("  --- pass %d/%d ---\n", pass + 1, options.Repeat);

			for (const auto& test : Registry::Get().Tests())
			{
				if (!Detail::Matches(test, options))
				{
					if (pass == 0)
						++filteredOut;
					continue;
				}

				TestResult result;
				result.Suite = test.Suite;
				result.Name = test.Name;
				result.TestType = test.TestType;

				if (breadcrumbs.is_open())
				{
					breadcrumbs << "RUNNING " << test.Suite << "::" << test.Name << "\n";
					breadcrumbs.flush();
				}

				Context::Get().Begin();
				CurrentTest() = &test;

				const auto start = std::chrono::high_resolution_clock::now();
				try
				{
					test.Fn();
				}
				catch (const TestFailure& f)
				{
					result.Failures.push_back(f.Message);
					result.Result = Outcome::Failed;
				}
				catch (const TestSkipped& s)
				{
					result.SkipReason = s.Reason;
					result.Result = Outcome::Skipped;
				}
				catch (const std::exception& e)
				{
					result.Failures.push_back(std::string("unexpected exception: ") + e.what());
					result.Result = Outcome::Failed;
				}
				catch (...)
				{
					result.Failures.push_back("unknown exception thrown");
					result.Result = Outcome::Failed;
				}
				const auto end = std::chrono::high_resolution_clock::now();

				CurrentTest() = nullptr;
				result.DurationMs = std::chrono::duration<double, std::milli>(end - start).count();

				// Soft failures count even when the body ran to completion.
				if (result.Result != Outcome::Skipped)
				{
					const auto& soft = Context::Get().SoftFailures();
					if (!soft.empty())
					{
						result.Failures.insert(result.Failures.begin(), soft.begin(), soft.end());
						result.Result = Outcome::Failed;
					}
				}

				const char* tag = result.Result == Outcome::Passed ? "PASS"
					: result.Result == Outcome::Skipped ? "SKIP" : "FAIL";

				std::printf("  [%s] [%-11s] %-52s %8.2f ms\n", tag, test.TestType,
					(std::string(test.Suite) + "::" + test.Name).c_str(), result.DurationMs);

				for (const auto& note : Context::Get().Notes())
					std::printf("         . %s\n", note.c_str());

				for (const auto& failure : result.Failures)
					std::printf("         ! %s\n", failure.c_str());

				if (result.Result == Outcome::Skipped)
					std::printf("         ~ %s\n", result.SkipReason.c_str());

				switch (result.Result)
				{
				case Outcome::Passed:  ++passed;  break;
				case Outcome::Failed:  ++failed;  break;
				case Outcome::Skipped: ++skipped; break;
				}

				if (breadcrumbs.is_open())
				{
					breadcrumbs << "  " << tag << " " << test.Suite << "::" << test.Name
						<< " (" << result.DurationMs << " ms)\n";

					// Mirror notes and failures into the log, not just stdout. A console window is
					// gone the moment it closes; this file is what actually gets read afterwards
					// (or pasted into a bug report), so it has to carry the WHY, not just the WHAT.
					for (const auto& note : Context::Get().Notes())
						breadcrumbs << "      . " << note << "\n";
					for (const auto& failure : result.Failures)
						breadcrumbs << "      ! " << failure << "\n";
					if (result.Result == Outcome::Skipped)
						breadcrumbs << "      ~ " << result.SkipReason << "\n";

					breadcrumbs.flush();
				}

				results.push_back(std::move(result));
				std::fflush(stdout);
			}
		}

		const auto suiteEnd = std::chrono::high_resolution_clock::now();
		const double totalMs = std::chrono::duration<double, std::milli>(suiteEnd - suiteStart).count();

		std::printf("--------------------------------------------\n");

		// Per-suite roll-up makes it obvious which subsystem regressed at a glance.
		std::vector<std::string> suiteOrder;
		for (const auto& r : results)
		{
			if (std::find(suiteOrder.begin(), suiteOrder.end(), r.Suite) == suiteOrder.end())
				suiteOrder.push_back(r.Suite);
		}
		for (const auto& suite : suiteOrder)
		{
			int suitePassed = 0, suiteFailed = 0, suiteSkipped = 0;
			double suiteMs = 0.0;
			for (const auto& r : results)
			{
				if (r.Suite != suite)
					continue;
				suiteMs += r.DurationMs;
				switch (r.Result)
				{
				case Outcome::Passed:  ++suitePassed;  break;
				case Outcome::Failed:  ++suiteFailed;  break;
				case Outcome::Skipped: ++suiteSkipped; break;
				}
			}
			std::printf("  %-22s %3d passed  %3d failed  %3d skipped  %8.2f ms\n",
				suite.c_str(), suitePassed, suiteFailed, suiteSkipped, suiteMs);
		}

		std::printf("--------------------------------------------\n");
		std::printf("  %d passed, %d failed, %d skipped", passed, failed, skipped);
		if (filteredOut)
			std::printf(", %d filtered out", filteredOut);
		std::printf("   [%.2f ms total]\n", totalMs);
		std::printf("  RESULT: %s\n", failed == 0 ? "OK" : "FAILURES");
		std::printf("============================================\n\n");
		std::fflush(stdout);

		if (!options.XmlOutputPath.empty())
			Detail::WriteJUnitXml(options.XmlOutputPath, results);
		if (!options.PerfCsvPath.empty() && !PerfSamples().empty())
			Detail::WritePerfCsv(options.PerfCsvPath, PerfSamples());

		if (breadcrumbs.is_open())
		{
			breadcrumbs << "DONE " << passed << " passed, " << failed << " failed, " << skipped << " skipped\n";
			breadcrumbs.flush();
		}

		return failed;
	}

	// Back-compat overload: RunAll("unit") still works.
	inline int RunAll(const char* typeFilter = nullptr)
	{
		RunOptions options;
		options.TypeFilter = typeFilter;
		return RunAll(options);
	}

	// Prints every registered test without running anything (EMBER_TEST_LIST=1).
	inline void ListTests()
	{
		std::printf("\n================ Ember-Test (list) ================\n");
		for (const auto& test : Registry::Get().Tests())
			std::printf("  [%-11s] %s::%s\n", test.TestType, test.Suite, test.Name);
		std::printf("  %zu tests registered\n", Registry::Get().Tests().size());
		std::printf("==================================================\n\n");
		std::fflush(stdout);
	}

} // namespace Ember::Test

// Defines a test body and self-registers it. `TestType` is one of Ember::Test::Type::*.
#define EB_TEST_CASE(Suite, Name, TestType)                                                             \
	static void Suite##_##Name##_body();                                                                \
	static ::Ember::Test::AutoRegister Suite##_##Name##_reg(#Suite, #Name, (TestType), Suite##_##Name##_body); \
	static void Suite##_##Name##_body()

//////////////////////////////////////////////////////////////////////////
// Hard assertions - abort the current test; the runner continues with the next one.
//////////////////////////////////////////////////////////////////////////

#define EB_CHECK(cond)           do { if (!(cond)) ::Ember::Test::ReportFail("CHECK(" #cond ")", __FILE__, __LINE__); } while (0)
#define EB_CHECK_FALSE(cond)     do { if ( (cond)) ::Ember::Test::ReportFail("CHECK_FALSE(" #cond ")", __FILE__, __LINE__); } while (0)
#define EB_CHECK_EQ(a, b)        do { if (!((a) == (b))) ::Ember::Test::ReportFail("CHECK_EQ(" #a ", " #b ")", __FILE__, __LINE__); } while (0)
#define EB_CHECK_NE(a, b)        do { if ( ((a) == (b))) ::Ember::Test::ReportFail("CHECK_NE(" #a ", " #b ")", __FILE__, __LINE__); } while (0)
#define EB_CHECK_MSG(cond, msg)  do { if (!(cond)) ::Ember::Test::ReportFail("CHECK(" #cond ")", __FILE__, __LINE__, (msg)); } while (0)

#define EB_CHECK_NEAR(a, b, eps) do {                                                                   \
		const double _ebA = (double)(a), _ebB = (double)(b);                                            \
		double _ebD = _ebA - _ebB; if (_ebD < 0) _ebD = -_ebD;                                          \
		if (!(_ebD <= (double)(eps)))                                                                   \
			::Ember::Test::ReportFail("CHECK_NEAR(" #a ", " #b ", " #eps ")", __FILE__, __LINE__,        \
				std::to_string(_ebA) + " vs " + std::to_string(_ebB) + " (delta " + std::to_string(_ebD) + ")"); \
	} while (0)

//////////////////////////////////////////////////////////////////////////
// Soft assertions - record the failure and keep going, so one run reports every broken value.
//////////////////////////////////////////////////////////////////////////

#define EB_EXPECT(cond)          do { if (!(cond)) ::Ember::Test::ReportSoftFail("EXPECT(" #cond ")", __FILE__, __LINE__); } while (0)
#define EB_EXPECT_FALSE(cond)    do { if ( (cond)) ::Ember::Test::ReportSoftFail("EXPECT_FALSE(" #cond ")", __FILE__, __LINE__); } while (0)
#define EB_EXPECT_EQ(a, b)       do { if (!((a) == (b))) ::Ember::Test::ReportSoftFail("EXPECT_EQ(" #a ", " #b ")", __FILE__, __LINE__); } while (0)
#define EB_EXPECT_NE(a, b)       do { if ( ((a) == (b))) ::Ember::Test::ReportSoftFail("EXPECT_NE(" #a ", " #b ")", __FILE__, __LINE__); } while (0)
#define EB_EXPECT_MSG(cond, msg) do { if (!(cond)) ::Ember::Test::ReportSoftFail("EXPECT(" #cond ")", __FILE__, __LINE__, (msg)); } while (0)

#define EB_EXPECT_NEAR(a, b, eps) do {                                                                  \
		const double _ebA = (double)(a), _ebB = (double)(b);                                            \
		double _ebD = _ebA - _ebB; if (_ebD < 0) _ebD = -_ebD;                                          \
		if (!(_ebD <= (double)(eps)))                                                                   \
			::Ember::Test::ReportSoftFail("EXPECT_NEAR(" #a ", " #b ", " #eps ")", __FILE__, __LINE__,   \
				std::to_string(_ebA) + " vs " + std::to_string(_ebB) + " (delta " + std::to_string(_ebD) + ")"); \
	} while (0)

// Ordered comparisons - soft by default since they usually appear in batches.
#define EB_EXPECT_GT(a, b)       do { if (!((a) >  (b))) ::Ember::Test::ReportSoftFail("EXPECT_GT(" #a ", " #b ")", __FILE__, __LINE__, std::to_string((double)(a)) + " vs " + std::to_string((double)(b))); } while (0)
#define EB_EXPECT_GE(a, b)       do { if (!((a) >= (b))) ::Ember::Test::ReportSoftFail("EXPECT_GE(" #a ", " #b ")", __FILE__, __LINE__, std::to_string((double)(a)) + " vs " + std::to_string((double)(b))); } while (0)
#define EB_EXPECT_LT(a, b)       do { if (!((a) <  (b))) ::Ember::Test::ReportSoftFail("EXPECT_LT(" #a ", " #b ")", __FILE__, __LINE__, std::to_string((double)(a)) + " vs " + std::to_string((double)(b))); } while (0)
#define EB_EXPECT_LE(a, b)       do { if (!((a) <= (b))) ::Ember::Test::ReportSoftFail("EXPECT_LE(" #a ", " #b ")", __FILE__, __LINE__, std::to_string((double)(a)) + " vs " + std::to_string((double)(b))); } while (0)

//////////////////////////////////////////////////////////////////////////
// Reporting helpers
//////////////////////////////////////////////////////////////////////////

// Marks the test as skipped (not failed) and stops it. For optional fixtures: a sample project that
// isn't checked in, an audio device that isn't present on a headless CI box, etc.
#define EB_SKIP(reason)          ::Ember::Test::ReportSkip((reason))

// Prints under the test line on pass AND fail. Use it for the numbers a human wants to eyeball.
#define EB_NOTE(msg)             ::Ember::Test::Context::Get().AddNote((msg))

//////////////////////////////////////////////////////////////////////////
// Benchmarking
//////////////////////////////////////////////////////////////////////////

// Times a block and soft-fails if the median exceeds budgetMs (scaled by EMBER_TEST_PERF_SCALE).
// Takes the block as __VA_ARGS__ so top-level commas inside it don't split into macro arguments.
//   EB_BENCH_BUDGET("label", 5.0, 30, { DoWork(); });
#define EB_BENCH_BUDGET(label, budgetMs, iterations, ...)                                               \
	do {                                                                                                \
		const ::Ember::Test::BenchmarkResult _ebResult =                                                \
			::Ember::Test::Benchmark([&]() __VA_ARGS__, (iterations));                                   \
		const double _ebBudget = (double)(budgetMs) * ::Ember::Test::PerfScale();                        \
		::Ember::Test::PerfSample _ebSample;                                                             \
		if (::Ember::Test::CurrentTest()) {                                                              \
			_ebSample.Suite = ::Ember::Test::CurrentTest()->Suite;                                        \
			_ebSample.Name = ::Ember::Test::CurrentTest()->Name;                                          \
		}                                                                                               \
		_ebSample.Label = (label);                                                                       \
		_ebSample.Result = _ebResult;                                                                    \
		_ebSample.BudgetMs = _ebBudget;                                                                  \
		::Ember::Test::PerfSamples().push_back(_ebSample);                                                \
		EB_NOTE(std::string(label) + ": " + _ebResult.ToString()                                          \
			+ "  [budget " + std::to_string(_ebBudget) + " ms]");                                         \
		EB_EXPECT_MSG(_ebResult.MedianMs <= _ebBudget,                                                    \
			std::string(label) + " median " + std::to_string(_ebResult.MedianMs)                          \
			+ " ms exceeded budget " + std::to_string(_ebBudget) + " ms");                                \
	} while (0)

// Same, but measurement-only: records and prints the numbers without asserting. Use while establishing
// a baseline on new hardware, or for figures that are informative but too noisy to gate on.
#define EB_BENCH_REPORT(label, iterations, ...)                                                         \
	do {                                                                                                \
		const ::Ember::Test::BenchmarkResult _ebResult =                                                \
			::Ember::Test::Benchmark([&]() __VA_ARGS__, (iterations));                                   \
		::Ember::Test::PerfSample _ebSample;                                                             \
		if (::Ember::Test::CurrentTest()) {                                                              \
			_ebSample.Suite = ::Ember::Test::CurrentTest()->Suite;                                        \
			_ebSample.Name = ::Ember::Test::CurrentTest()->Name;                                          \
		}                                                                                               \
		_ebSample.Label = (label);                                                                       \
		_ebSample.Result = _ebResult;                                                                    \
		::Ember::Test::PerfSamples().push_back(_ebSample);                                                \
		EB_NOTE(std::string(label) + ": " + _ebResult.ToString());                                        \
	} while (0)
