# Ember-Test

A standalone test executable for the engine. It links `Ember`, boots a minimal `Application` (hidden
window + GL context + default assets), runs every registered test on the first frame, and exits with
`0` (all passed) or `1` (any failure) so CI can gate on it.

It exists both to test the engine and as **scaffolding**: copy the patterns here for new tests.

## Running

Build the `Ember-Test` project (Debug **and** Release — the whole point is catching Release-only bugs),
then run the executable. Do it in **both** configs in CI.

Environment variables:

| Var | Effect |
|---|---|
| `EMBER_TEST_FILTER=unit\|integration\|visual\|performance` | Run only one category (e.g. fast unit tests per-commit, visual tests nightly). |
| `EMBER_TEST_WRITE_GOLDEN=1` | Regenerate visual reference images instead of comparing. |

## Test categories (and where each lives)

| Type | File | What it covers | Needs a running engine? |
|---|---|---|---|
| **unit** | `src/Tests/UnitTests.cpp` | Pure logic — math, UUIDs, transform decompose. Fast, deterministic, most numerous. | No |
| **integration** | `src/Tests/IntegrationTests.cpp` | Seams between subsystems — ECS lifecycle, `TransformSystem`, scene YAML round-trip. | Yes (asset manager + systems) |
| **visual** | `src/Tests/VisualTests.cpp` | Golden-image: render a fixed scene, compare pixels to a committed reference. Catches gross render corruption regardless of cause. | Yes (+ GL context) |
| **performance** | `src/Tests/IntegrationTests.cpp` | Assert a task stays within a time budget. Machine-dependent — keep budgets generous. | Yes |

## Writing a test

```cpp
#include <Ember.h>
#include "TestFramework.h"

EB_TEST_CASE(SuiteName, TestName, Ember::Test::Type::Unit)
{
    EB_CHECK(1 + 1 == 2);
    EB_CHECK_NEAR(value, expected, 1e-5);
    EB_CHECK_MSG(cond, "context shown on failure");
}
```

Tests self-register via a static initializer — no manual list to maintain. Assertions throw to abort the
current test; the runner records the failure and continues to the next one.

## Golden-image workflow

1. Mint the reference once, in a **known-good** build: set `EMBER_TEST_WRITE_GOLDEN=1` and run.
2. Commit `Ember-Test/golden/*.png`.
3. Normal runs compare against it. A mismatch fails with the diff percentage. If a visual change is
   intentional, regenerate and re-commit.

Tolerances (`channelTolerance`, `maxDiffFraction` in `VisualTests.cpp`) are deliberately loose to absorb
tiny Debug-vs-Release floating-point differences while still catching blowouts / channel shifts / black
frames. Tighten them as the pipeline stabilizes.

## Notes / next steps

- Loading `.ebs` scenes directly (rather than code-built scenes) is a natural next set of tests — the
  serialization round-trip test is the seed for that.
- For a richer framework (fixtures, parameterized tests, better reporting), drop in single-header
  **doctest** or **Catch2** and keep the same test bodies.
- `RenderSystem::CaptureBackbufferToPNG` doubles as a screenshot utility outside of testing.
