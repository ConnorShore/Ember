// The in-memory log ring that backs Ember-Forge's Log panel. Every test syncs its cursor to the
// current head first, because the suite itself is logging constantly around these.

#include <Ember.h>

#include "TestFramework.h"

#include <algorithm>
#include <string>
#include <vector>

using namespace Ember;
using Ember::Test::Type::Unit;

namespace {

	// Advances past everything logged so far and returns a cursor positioned at the head.
	uint64_t SyncedCursor()
	{
		uint64_t cursor = 0;
		std::vector<LogRecord> discarded;
		Logger::DrainRecords(cursor, discarded);

		return cursor;
	}

	bool ContainsMessage(const std::vector<LogRecord>& records, const std::string& message)
	{
		return std::ranges::any_of(records, [&](const LogRecord& record) { return record.Message == message; });
	}

}

EB_TEST_CASE(Logger, DrainReturnsRecordsNewerThanTheCursor, Unit)
{
	uint64_t cursor = SyncedCursor();

	EB_CORE_INFO("LoggerTests: drain marker");

	std::vector<LogRecord> records;
	Logger::DrainRecords(cursor, records);

	EB_CHECK(ContainsMessage(records, "LoggerTests: drain marker"));
}

EB_TEST_CASE(Logger, DrainingTwiceReturnsNothingTheSecondTime, Unit)
{
	uint64_t cursor = SyncedCursor();

	EB_CORE_INFO("LoggerTests: cursor advance marker");

	std::vector<LogRecord> first;
	Logger::DrainRecords(cursor, first);
	EB_CHECK(!first.empty());

	// A drained cursor must sit at the head, or the panel would replay its whole backlog each frame.
	std::vector<LogRecord> second;
	Logger::DrainRecords(cursor, second);
	EB_CHECK(second.empty());
}

EB_TEST_CASE(Logger, IndependentCursorsBothSeeTheSameRecords, Unit)
{
	// The drain is non-destructive so a panel created late still sees startup lines, and a second
	// consumer does not steal records from the first.
	uint64_t panelCursor = SyncedCursor();
	uint64_t otherCursor = panelCursor;

	EB_CORE_WARN("LoggerTests: shared marker");

	std::vector<LogRecord> panelRecords;
	Logger::DrainRecords(panelCursor, panelRecords);

	std::vector<LogRecord> otherRecords;
	Logger::DrainRecords(otherCursor, otherRecords);

	EB_CHECK(ContainsMessage(panelRecords, "LoggerTests: shared marker"));
	EB_CHECK(ContainsMessage(otherRecords, "LoggerTests: shared marker"));
}

EB_TEST_CASE(Logger, RecordsCarryLevelAndSourceSeparately, Unit)
{
	// The panel colours and filters off these fields; flattening them into the message would put it
	// back to parsing its own formatted output.
	uint64_t cursor = SyncedCursor();

	EB_CORE_ERROR("LoggerTests: engine error marker");
	EB_WARN("LoggerTests: app warn marker");

	std::vector<LogRecord> records;
	Logger::DrainRecords(cursor, records);

	const auto engine = std::ranges::find_if(records,
		[](const LogRecord& record) { return record.Message == "LoggerTests: engine error marker"; });
	const auto app = std::ranges::find_if(records,
		[](const LogRecord& record) { return record.Message == "LoggerTests: app warn marker"; });

	EB_CHECK(engine != records.end());
	EB_CHECK(app != records.end());

	if (engine == records.end() || app == records.end())
		return;

	EB_CHECK(engine->Level == LogLevel::Error);
	EB_CHECK_EQ(engine->Logger, std::string("ENGINE"));

	EB_CHECK(app->Level == LogLevel::Warn);
	EB_CHECK_EQ(app->Logger, std::string("APP"));

	// Ordering is what lets the cursor convert straight to a deque index.
	EB_CHECK(engine->Sequence < app->Sequence);
}

EB_TEST_CASE(Logger, DrainFromZeroReturnsTheWholeSurvivingRing, Unit)
{
	// A cursor older than anything still buffered must clamp to the front rather than index off it.
	// Only hits the post-eviction path once the suite has logged past the ring's capacity.
	std::vector<LogRecord> records;
	uint64_t cursor = 0;
	Logger::DrainRecords(cursor, records);

	EB_CHECK(!records.empty());
	EB_CHECK(std::ranges::is_sorted(records, {}, &LogRecord::Sequence));
}
