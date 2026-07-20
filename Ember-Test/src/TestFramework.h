#pragma once

// A deliberately tiny, dependency-free test framework (no engine, no vendored lib). It exists so this
// project can demonstrate the *shape* of engine tests without pulling in Catch2/doctest — swap it for
// one of those later if you want fixtures/parameterization; the EB_TEST_CASE / EB_CHECK surface is
// intentionally close to theirs so tests port easily.
//
// Usage:
//   EB_TEST_CASE(Math, LerpMidpoint, Ember::Test::Type::Unit)
//   {
//       EB_CHECK_NEAR(Ember::Math::Lerp(0.0f, 10.0f, 0.5f), 5.0f, 1e-5);
//   }
//
// A test "fails" by throwing (via EB_CHECK*); the runner catches it, records the failure, and moves on
// to the next test. RunAll() returns the failed-test count so main() can use it as the process exit code.

#include <cstdio>
#include <exception>
#include <functional>
#include <string>
#include <vector>

namespace Ember::Test {

	// The categories a game engine wants coverage in. Filterable at run time so CI can, e.g., run only
	// the fast unit tests on every commit and the slower visual tests nightly.
	namespace Type {
		inline constexpr const char* Unit = "unit";               // pure logic, no engine subsystems
		inline constexpr const char* Integration = "integration"; // multiple subsystems together, no rendering
		inline constexpr const char* Visual = "visual";           // renders a frame and compares pixels
		inline constexpr const char* Performance = "performance"; // asserts on a timing/perf budget
	}

	struct TestFailure { std::string Message; };

	struct TestCase {
		const char* Suite;
		const char* Name;
		const char* TestType;
		std::function<void()> Fn;
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

	[[noreturn]] inline void ReportFail(const char* expr, const char* file, int line, const std::string& extra = {}) {
		std::string msg = std::string(file) + "(" + std::to_string(line) + "): " + expr;
		if (!extra.empty())
			msg += "  -> " + extra;
		throw TestFailure{ msg };
	}

	// Runs all registered tests (optionally filtered to a single Type). Prints a per-test PASS/FAIL line
	// plus a summary to stdout, and returns the number of FAILED tests (0 == everything passed) so it can
	// be used directly as a process exit code in CI.
	inline int RunAll(const char* typeFilter = nullptr) {
		int passed = 0, failed = 0, skipped = 0;
		std::printf("\n================ Ember-Test ================\n");
		for (const auto& t : Registry::Get().Tests()) {
			if (typeFilter && std::string(typeFilter) != t.TestType) { ++skipped; continue; }
			try {
				t.Fn();
				std::printf("  [PASS] [%-11s] %s::%s\n", t.TestType, t.Suite, t.Name);
				++passed;
			}
			catch (const TestFailure& f) {
				std::printf("  [FAIL] [%-11s] %s::%s\n         %s\n", t.TestType, t.Suite, t.Name, f.Message.c_str());
				++failed;
			}
			catch (const std::exception& e) {
				std::printf("  [FAIL] [%-11s] %s::%s\n         unexpected exception: %s\n", t.TestType, t.Suite, t.Name, e.what());
				++failed;
			}
			catch (...) {
				std::printf("  [FAIL] [%-11s] %s::%s\n         unknown exception thrown\n", t.TestType, t.Suite, t.Name);
				++failed;
			}
		}
		std::printf("--------------------------------------------\n");
		std::printf("  %d passed, %d failed", passed, failed);
		if (skipped) std::printf(", %d skipped", skipped);
		std::printf("\n============================================\n\n");
		std::fflush(stdout);
		return failed;
	}

} // namespace Ember::Test

// Defines a test body and self-registers it. `Type` is one of Ember::Test::Type::*.
#define EB_TEST_CASE(Suite, Name, TestType)                                                             \
	static void Suite##_##Name##_body();                                                                \
	static ::Ember::Test::AutoRegister Suite##_##Name##_reg(#Suite, #Name, (TestType), Suite##_##Name##_body); \
	static void Suite##_##Name##_body()

// Assertions. Each aborts the *current* test on failure (throws); the runner keeps going with the rest.
#define EB_CHECK(cond)           do { if (!(cond)) ::Ember::Test::ReportFail("CHECK(" #cond ")", __FILE__, __LINE__); } while (0)
#define EB_CHECK_FALSE(cond)     do { if ( (cond)) ::Ember::Test::ReportFail("CHECK_FALSE(" #cond ")", __FILE__, __LINE__); } while (0)
#define EB_CHECK_EQ(a, b)        do { if (!((a) == (b))) ::Ember::Test::ReportFail("CHECK_EQ(" #a ", " #b ")", __FILE__, __LINE__); } while (0)
#define EB_CHECK_NEAR(a, b, eps) do { double _d = (double)(a) - (double)(b); if (_d < 0) _d = -_d; if (_d > (double)(eps)) ::Ember::Test::ReportFail("CHECK_NEAR(" #a ", " #b ", " #eps ")", __FILE__, __LINE__, std::to_string((double)(a)) + " vs " + std::to_string((double)(b))); } while (0)
#define EB_CHECK_MSG(cond, msg)  do { if (!(cond)) ::Ember::Test::ReportFail("CHECK(" #cond ")", __FILE__, __LINE__, (msg)); } while (0)
